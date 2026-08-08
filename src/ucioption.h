// ucioption.h — UCI Options management.
// Allows the GUI to configure engine parameters (Hash, Threads, etc.)
#ifndef LOFTY_UCIOPTION_H
#define LOFTY_UCIOPTION_H

#include <string>
#include <map>
#include <functional>

namespace lofty {

enum class OptionType { CHECK, SPIN, STRING, BUTTON, COMBO };

struct Option {
    OptionType type;
    std::string defaultValue;
    std::string currentValue;
    int min = 0;
    int max = 0;
    std::function<void(const Option&)> on_change;

    Option() = default;
    
    // Constructor for Check (boolean)
    Option(bool v, std::function<void(const Option&)> f = {});
    
    // Constructor for Spin (integer)
    Option(int v, int minv, int maxv, std::function<void(const Option&)> f = {});
    
    // Constructor for String
    Option(const char* v, std::function<void(const Option&)> f = {});
    
    // Constructor for Button (action trigger)
    Option(std::function<void(const Option&)> f);

    int as_int() const;
    bool as_bool() const;
    std::string as_string() const;
};

class OptionsMap {
    std::map<std::string, Option> options;

public:
    void add(const std::string& name, const Option& opt);
    void set(const std::string& name, const std::string& value);
    const Option& get(const std::string& name) const;
    void print() const; // Outputs "option name ..." for UCI
};

extern OptionsMap Options; // Global options instance

// init_options — registers all default options.
void init_options(OptionsMap& options);

} // namespace lofty

#endif // LOFTY_UCIOPTION_H