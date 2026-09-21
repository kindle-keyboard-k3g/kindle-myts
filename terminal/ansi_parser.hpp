#ifndef MYTS_TERMINAL_ANSI_PARSER_HPP
#define MYTS_TERMINAL_ANSI_PARSER_HPP

#include <string>
#include <string_view>
#include <cstdlib>
#include <cctype>

namespace myts {
namespace terminal {

/**
 * @brief Abstract sink interface receiving parsed ANSI terminal actions.
 */
class IAnsiHandler {
public:
    virtual ~IAnsiHandler() = default;

    /**
     * @brief Invoked when cursor position sequence (CUP) is decoded.
     */
    virtual void on_cursor_move(int row, int col) = 0;

    /**
     * @brief Invoked when erase display command (ED) is decoded.
     */
    virtual void on_erase_display(int mode) = 0;

    /**
     * @brief Invoked for regular printable characters.
     */
    virtual void on_print_char(char c) = 0;
};

/**
 * @brief Incremental state machine for VT100 / ANSI CSI escape sequence parsing.
 *
 * Buffers partial escape sequences across read boundaries to guarantee robust stream decoding.
 */
class AnsiParser {
public:
    enum class State {
        Ground,
        Escape,
        Csi
    };

    /**
     * @brief Constructs parser binding it to an output handler.
     * @param handler Reference to terminal handler sink.
     */
    explicit AnsiParser(IAnsiHandler& handler) noexcept
        : handler_(handler), state_(State::Ground) {}

    /**
     * @brief Feeds incoming byte slice into parser state machine.
     * @param input Stream chunk to process.
     */
    void feed(std::string_view input) {
        for (char c : input) {
            switch (state_) {
            case State::Ground:
                if (c == '\033') {
                    state_ = State::Escape;
                } else {
                    handler_.on_print_char(c);
                }
                break;

            case State::Escape:
                if (c == '[') {
                    state_ = State::Csi;
                    param_buf_.clear();
                } else {
                    // Not a CSI sequence, return to ground
                    state_ = State::Ground;
                }
                break;

            case State::Csi:
                if (std::isdigit(static_cast<unsigned char>(c)) || c == ';') {
                    param_buf_.push_back(c);
                } else {
                    // Command terminator
                    execute_csi(c);
                    state_ = State::Ground;
                    param_buf_.clear();
                }
                break;
            }
        }
    }

private:
    void execute_csi(char cmd) {
        int a1 = 0;
        int a2 = 0;

        size_t sep = param_buf_.find(';');
        if (sep != std::string::npos) {
            a1 = std::atoi(param_buf_.substr(0, sep).c_str());
            a2 = std::atoi(param_buf_.substr(sep + 1).c_str());
        } else if (!param_buf_.empty()) {
            a1 = std::atoi(param_buf_.c_str());
        }

        switch (cmd) {
        case 'H':
        case 'f':
            handler_.on_cursor_move(a1, a2);
            break;
        case 'J':
            handler_.on_erase_display(a1);
            break;
        default:
            break;
        }
    }

    IAnsiHandler& handler_;
    State state_;
    std::string param_buf_;
};

} // namespace terminal
} // namespace myts

#endif // MYTS_TERMINAL_ANSI_PARSER_HPP
