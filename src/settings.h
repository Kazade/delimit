#ifndef SETTINGS_H
#define SETTINGS_H

namespace delimit {

/*
 *  Incharge of reading settings from settings files
 *
 *  Usage:
 *
 *  settings.read_setting("trim_trailing_whitespace", "text/plain")
 *
 *  Will look for:
 *
 *  ~/.config/delimit/settings/text_plain.json
 *  /usr/share/delimit/settings/text_plain.json
 *  ~/.config/delimit/settings/default.json
 *  /usr/share/delimit/settings/default.json
 *
 */

class Settings {
public:
    Settings();

    template<typename T>
    void read_setting(const unicode& mime_type, const unicode& setting, T& output) {
        auto it = mime_settings_.find(mime_type);
        if(it != mime_settings_.end()) {
            //OK, we have some settings for this mime type
        } else {
            //Do the settings exist? Can we load them?
            load_settings(mime_type);
            it = mime_settings_.find(mime_type);
            if(it != mime_settings_.end()) {
                //OK, we were able to load some, does the setting we want exist?

                auto found = (*it).second.find(setting);
                if(found != (*it).second.end()) {
                    //Yay, we have our setting
                    output = (*found).second;
                }
            }
        }
    }

private:
    std::map<unicode, std::map<unicode, unicode>> mime_settings_;
    std::map<unicode, unicode> default_settings_;
};

}


#endif // SETTINGS_H
