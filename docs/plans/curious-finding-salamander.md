# Plano: Refresh Local, Fontes Mais Grossas e Script de Animação ASCII

## 1. Contexto e Diagnóstico dos Problemas

### Problema 1 — Tela piscando branco em cada tecla (CRÍTICO)

**Causa raiz confirmada:** `app/application.hpp` `render_frame()` marca o canvas inteiro como sujo em cada frame:
```cpp
graphics::Rect dirty(0, 0, driver_.width(), driver_.height());   // SEMPRE 600×800!
display_.mark_dirty(dirty);
display_.flush();
```
O `should_escalate_to_full()` em `EinkDisplay` calcula:
```cpp
float ratio = area.width * area.height / total_pixels;   // = 480000 / 480000 = 1.0
return ratio >= 0.5f;  // SEMPRE true
```
Resultado: **cada tecla chama `driver_.update_display_full()` → GC16 full flash** (branco + preto ~800ms). Nunca usa `update_display_area()`.

**Solução:** `TerminalSession` deve rastrear quais linhas mudaram (`dirty_rows_`), e `render_frame()` computa o `Rect` mínimo a partir apenas dessas linhas. Linha não-dirty não entra no dirty rect. `update_display_area()` é chamado apenas na área alterada, eliminando o flash.

### Problema 2 — Fonte fina demais / difícil de ler no e-ink

Único font existente: `ter-u12n.hex` (Terminus Normal, 8×12 px). Terminus bold `ter-u16b.hex` (8×16) ou `ter-u20b.hex` (10×20) são ideais. Alternativa: **síntese de bold** via bit-OR horizontal dentro de `FontRenderer::draw_char_bold()` — opera sobre os bytes do glyph já carregado sem arquivos extras: `row |= (row >> 1)`.

### Problema 3 — Sem ferramenta para medir qualidade visual

Não existe script de animação ASCII. Precisamos de `tools/ascii-anim.sh` que rode no Kindle via PTY, desenhando frames com escapes ANSI para exercitar dirty-rect parcial, scroll, e cursor.

---

## 2. Implementação Detalhada

### Fase 1: Dirty Tracking por Linha em `TerminalSession`

**Arquivo:** `terminal/terminal_session.hpp`

1. Adicionar `dirty_rows_` (`std::vector<bool>`, tamanho `rows_`), inicializado `true` (primeiro render completo).
2. Adicionar `take_dirty_rows()` que retorna e reseta o vetor.
3. Marcar `dirty_rows_[cursor_row_] = true` em:
   - `on_print_char()` (antes e depois se rolar)
   - `on_erase_display()` (todas as linhas afetadas)
   - `scroll_up()` (todas as linhas)
   - `on_cursor_move()` (linha cursor anterior — para cursor erase)
4. `render()` aceita parâmetro opcional `const std::vector<bool>* dirty_rows = nullptr` — se fornecido, renderiza apenas as linhas marcadas.

**Arquivo:** `app/application.hpp` — `render_frame()`:
```cpp
void render_frame() {
    auto dirty_rows = session_.take_dirty_rows();
    graphics::Rect dirty = compute_dirty_rect(dirty_rows);
    if (dirty.is_empty()) { return; }  // nada mudou

    session_.render(canvas_, font_, /*show_cursor=*/true, &dirty_rows);

    if (debug_cfg_.enable_overlay) {
        overlay_.render(canvas_, font_, metrics_.snapshot(), 0, 0);
        dirty = dirty.union_with(Rect{0, 0, driver_.width(), font_.glyph_height() * 2});
    }

    uint8_t* dst = driver_.surface_data();
    if (dst && canvas_.data()) { std::memcpy(dst, canvas_.data(), canvas_.size()); }

    display_.mark_dirty(dirty);
    bool was_full = (display_.partial_updates_count() + 1 >= display_.refresh_config().partial_limit);
    metrics_.record_refresh(dirty, was_full);
    display_.flush();
}

graphics::Rect compute_dirty_rect(const std::vector<bool>& dirty_rows) const noexcept {
    int gh = font_.glyph_height();
    int first = -1, last = -1;
    for (int r = 0; r < static_cast<int>(dirty_rows.size()); ++r) {
        if (!dirty_rows[r]) { continue; }
        if (first < 0) { first = r; }
        last = r;
    }
    if (first < 0) { return Rect{0,0,0,0}; }
    return Rect{0, first * gh, driver_.width(), (last - first + 1) * gh};
}
```

Agora um pressionamento de tecla que altera apenas a linha do cursor gera um `Rect` de ~12px de altura ao invés de 800px, e `update_display_area()` é chamado. Sem flash.

**Ajustar `RefreshConfig` defaults:**
- `partial_limit = 40` (era 15 — 40 atualizações parciais antes do GC16 de limpeza)
- `dirty_ratio_threshold = 0.85f` (era 0.5f — só escala para full quando >85% da tela)

### Fase 2: Bold Font

**Opção A (preferida):** Baixar `ter-u16b.hex` (Terminus 8×16 bold) de https://terminus-font.sourceforge.net/ e incluir no repositório. Lançar via `myts-ng-dbg /dev/fb0 ter-u16b.hex`.

**Opção B (sem arquivo externo):** Adicionar `draw_char_bold()` em `graphics/font_renderer.hpp`:
```cpp
bool draw_char_bold(OwnedPixmap& dst, int x, int y, uint32_t codepoint,
                    uint8_t fg = 0x00, uint8_t bg = 0x0F) const noexcept {
    GlyphBitmap glyph = get_glyph(codepoint);
    if (!glyph.data) { return false; }
    for (int r = 0; r < height_; ++r) {
        const uint8_t* src = glyph.data + (r * bytes_per_row_);
        for (int c = 0; c < width_; ++c) {
            int bidx = c / 8, bit = 7 - (c % 8);
            uint8_t row_bold = src[bidx] | (src[bidx] >> 1);
            bool is_fg = (row_bold & (1 << bit)) != 0;
            dst.set_pixel(x + c, y + r, is_fg ? fg : bg);
        }
    }
    return true;
}
```
Pode ser ligado com `--bold` flag em `main.cpp` → `Application` passa `use_bold_ = true` para `TerminalSession::render()`.

**Tanto A quanto B devem ser implementados:** A (arquivo `.hex` 8×16 bold) para terminais reais, B (bold sintético) como fallback.

### Fase 3: Script de Animação ASCII

**Arquivo:** `tools/ascii-anim.sh` — roda no Kindle via SSH ou direto no PTY:
```bash
#!/usr/bin/env sh
# Exercita dirty-rect parcial, scroll, e cursor no kindle-myts.
# Roda na shell gerenciada pelo PTY do myts-ng.
DELAY=0.2
FRAMES=50
clear
for i in $(seq 1 $FRAMES); do
    # Mover cursor para linha 0, col 0
    printf '\033[H'
    printf "Frame %03d/%03d | e-ink dirty-rect test\n" "$i" "$FRAMES"
    # Spinner na linha 1
    spin="-\\|/"
    c=$(( i % 4 ))
    printf "  Spinner: %s\n" "$(echo "$spin" | cut -c$((c+1)))"
    # Barra de progresso linha 2
    filled=$(( i * 30 / FRAMES ))
    bar=""; j=0
    while [ $j -lt 30 ]; do
        [ $j -lt $filled ] && bar="${bar}#" || bar="${bar}."
        j=$((j+1))
    done
    printf "  [%s]\n" "$bar"
    # Linha de rolagem no fundo
    printf "  Rolling text: %s\n" "$(date)"
    sleep $DELAY
done
printf '\nDone.\n'
```

Lançar no Kindle: `ssh kindle "cd /mnt/us/myts && sh /mnt/us/myts/ascii-anim.sh"` (ou digitar no terminal interativo).

---

## 3. Arquivos Modificados

| Arquivo | Mudança |
|---|---|
| `terminal/terminal_session.hpp` | `dirty_rows_` tracking, `take_dirty_rows()`, `render()` com dirty hint |
| `app/application.hpp` | `compute_dirty_rect()`, `render_frame()` usa dirty rect real, ajustar `RefreshConfig` |
| `graphics/font_renderer.hpp` | `draw_char_bold()` via bit-OR horizontal |
| `graphics/eink_display.hpp` | `partial_limit=40`, `dirty_ratio_threshold=0.85f` |
| `tests/test_eink_display.cpp` | Atualizar valores de default nos testes existentes |
| `tests/test_terminal_session.cpp` | Testes para `dirty_rows_` tracking e `take_dirty_rows()` |
| `tools/ascii-anim.sh` | Script de animação ASCII |
| `Makefile` | Rebuilt cross-compile targets |

---

## 4. Ordem de Execução (TDD)

1. **Red:** Adicionar testes para `take_dirty_rows()` e dirty tracking — falham.
2. **Green:** Implementar `dirty_rows_` em `TerminalSession`.
3. **Red:** Testes para `compute_dirty_rect()` em Application — falham.
4. **Green:** Implementar `compute_dirty_rect()` e `render_frame()` corrigido.
5. **Red:** Testes para `draw_char_bold()` — falham.
6. **Green:** Implementar `draw_char_bold()`.
7. Cross-compilar, transferir, lançar no Kindle, capturar BMP com `./tools/capture-kindle-fb.sh`.
8. Rodar `tools/ascii-anim.sh` no PTY do Kindle para validar qualidade visual.

---

## 5. Verificação

- `make test` — suite completa 45+ testes passando.
- `make test-asan` — sem leaks ou UB.
- Cross-compile: `make myts-ng-dbg` com toolchain musl ARMv6.
- Captura BMP antes e depois: `./tools/capture-kindle-fb.sh /tmp/before.bmp` e `/tmp/after.bmp`.
- Comparar: nenhuma mancha de ghosting, texto legível, sem flash branco ao digitar.
