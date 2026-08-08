// ucioption.cpp — UCI Options implementation.
#include "ucioption.h"
#include "tt.h"
#include "syzygy.h"

#include <iostream>
#include <cstdlib>

namespace lofty {

OptionsMap Options;

Option::Option(bool v, std::function<void(const Option&)> f) :
    type(OptionType::CHECK), on_change(f) {
    defaultValue = v ? "true" : "false";
    currentValue = defaultValue;
}

Option::Option(int v, int minv, int maxv, std::function<void(const Option&)> f) :
    type(OptionType::SPIN), min(minv), max(maxv), on_change(f) {
    defaultValue = std::to_string(v);
    currentValue = defaultValue;
}

Option::Option(const char* v, std::function<void(const Option&)> f) :
    type(OptionType::STRING), on_change(f) {
    defaultValue = v;
    currentValue = v;
}

Option::Option(std::function<void(const Option&)> f) :
    type(OptionType::BUTTON), on_change(f) {}

int Option::as_int() const { return std::atoi(currentValue.c_str()); }
bool Option::as_bool() const { return currentValue == "true"; }
std::string Option::as_string() const { return currentValue; }

void OptionsMap::add(const std::string& name, const Option& opt) {
    options[name] = opt;
}

void OptionsMap::set(const std::string& name, const std::string& value) {
    auto it = options.find(name);
    if (it != options.end()) {
        Option& opt = it->second;
        
        if (opt.type == OptionType::CHECK) {
            if (value == "true" || value == "false") {
                opt.currentValue = value;
                if (opt.on_change) opt.on_change(opt);
            }
        } else if (opt.type == OptionType::SPIN) {
            int val = std::atoi(value.c_str());
            if (val >= opt.min && val <= opt.max) {
                opt.currentValue = std::to_string(val);
                if (opt.on_change) opt.on_change(opt);
            }
        } else if (opt.type == OptionType::STRING) {
            opt.currentValue = value;
            if (opt.on_change) opt.on_change(opt);
        } else if (opt.type == OptionType::BUTTON) {
            if (opt.on_change) opt.on_change(opt);
        }
    }
}

const Option& OptionsMap::get(const std::string& name) const {
    return options.at(name);
}

void OptionsMap::print() const {
    for (const auto& pair : options) {
        const Option& o = pair.second;
        std::cout << "option name " << pair.first << " type ";
        switch (o.type) {
            case OptionType::CHECK: std::cout << "check default " << o.defaultValue; break;
            case OptionType::SPIN: std::cout << "spin default " << o.defaultValue << " min " << o.min << " max " << o.max; break;
            case OptionType::STRING: std::cout << "string default " << (o.defaultValue.empty() ? "<empty>" : o.defaultValue); break;
            case OptionType::BUTTON: std::cout << "button"; break;
            case OptionType::COMBO: std::cout << "combo default " << o.defaultValue; break;
        }
        std::cout << "\n";
    }
}

void init_options(OptionsMap& options) {
    // Changed default Hash from 64 to 256MB for deeper search tree storage
    options.add("Hash", Option(256, 1, 33554432, [](const Option& o){
        TT.resize(o.as_int());
        TT.clear();
    }));
    
    options.add("Clear Hash", Option(std::function<void(const Option&)>([](const Option&){
        TT.clear();
    })));
    
    options.add("Threads", Option(1, 1, 512, [](const Option&){}));
    
    // --- Syzygy Tablebases ---
    options.add("SyzygyPath", Option("<empty>", [](const Option& o){
        lofty::init_tb(o.as_string());
    }));
    options.add("SyzygyProbeLimit", Option(7, 0, 7, [](const Option&){}));
    options.add("SyzygyProbeDepth", Option(1, 1, 100, [](const Option&){}));
    options.add("Syzygy50MoveRule", Option(true, [](const Option&){}));
}

} // namespace lofty