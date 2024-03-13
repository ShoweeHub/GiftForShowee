class ConfigItem {
private:
    static const String inputElementTemplate;
public:
    String name;
    String alias;
    String value;
    String placeholder;
    String pattern;
    bool required;
    bool notBlank;

    ConfigItem(const String &name, const String &alias, const String &value, const String &placeholder, const String &pattern, bool required, bool notBlank) {
        this->name = name;
        this->alias = alias;
        this->value = value;
        this->placeholder = placeholder;
        this->pattern = pattern;
        this->required = required;
        this->notBlank = notBlank;
    }

    String getInputElement() const {
        String result = inputElementTemplate;
        result.replace("'", "[TempAposForReplace]");
        result.replace("[ALIAS]", alias);
        result.replace("[NAME]", name);
        result.replace("[REQUIRED] ", required ? "required " : "");
        result.replace("[VALUE]", value);
        result.replace("[PLACEHOLDER]", placeholder);
        result.replace("[PATTERN]", pattern);
        result.replace("'", "&apos;");
        result.replace("[TempAposForReplace]", "'");
        return result;
    }
};

class Config {
private:
    static const String configPageHtmlTemplate;
    static const String homepageButtonTemplate;

    static String JsonStringOr(const JsonVariant &v, const String &def, bool notBlank) {
        return v.isNull() || (notBlank && v.as<String>().length() == 0) ? def : v.as<String>();
    }

public:
    String name;
    String alias;
    std::vector<ConfigItem> items;

    Config(const String &name, const String &alias, const std::vector<ConfigItem> &items) {
        this->name = name;
        this->alias = alias;
        this->items = items;
    }

    void loadConfigFromServerArgs(WebServer &server) {
        for (auto &item: items) {
            item.value = server.arg(item.name);
        }
    }

    bool loadConfig() {
        Serial.printf("正在加载%s配置...\n", alias.c_str());
        File configFile = LittleFS.open("/" + name + "Config.json", "r");
        if (!configFile) {
            Serial.printf("未找到%s配置文件\n", alias.c_str());
            return false;
        }
        String config = configFile.readString();
        configFile.close();
        Serial.printf("%s配置文件内容: %s\n", alias.c_str(), config.c_str());
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, config);
        if (error) {
            Serial.printf("解析%s配置文件失败\n", alias.c_str());
            return false;
        }
        for (auto &item: items) {
            item.value = JsonStringOr(doc[item.name], item.value, item.notBlank);
        }
        Serial.printf("%s配置加载成功\n", alias.c_str());
        return true;
    }

    bool saveConfig() {
        Serial.printf("正在保存%s配置...\n", alias.c_str());
        JsonDocument doc;
        for (auto &item: items) {
            doc[item.name] = item.value;
        }
        File configFile = LittleFS.open("/" + name + "Config.json", "w");
        if (!configFile) {
            Serial.printf("打开%s配置文件失败,此错误无法修复\n", alias.c_str());
            return false;
        }
        serializeJson(doc, configFile);
        configFile.close();
        Serial.printf("%s配置保存成功\n", alias.c_str());
        return true;
    }

    String &operator[](const String &itemName) {
        for (auto &item: items) {
            if (item.name == itemName) {
                return item.value;
            }
        }
        throw std::out_of_range("未找到配置项");
    }

    String getConfigPageHtml() const {
        String result = configPageHtmlTemplate;
        result.replace("[ALIAS]", alias);
        result.replace("[NAME]", name);
        String inputElements;
        for (auto &item: items) {
            inputElements += item.getInputElement();
            inputElements.trim();
        }
        result.replace("[ITEMS]", inputElements);
        result.trim();
        return result;
    }

    String getHomePageButton() const {
        String result = homepageButtonTemplate;
        result.replace("[ALIAS]", alias);
        result.replace("[NAME]", name);
        return result;
    }
};

class HomePage {
private:
    static const String homePageHtmlTemplate;
public:
    static String homePageHtml;

    static void initHomePageHtml(const String &hostName, const std::vector<Config *> &configs) {
        homePageHtml = homePageHtmlTemplate;
        String buttons;
        for (auto config: configs) {
            buttons += config->getHomePageButton();
            buttons.trim();
        }
        homePageHtml.replace("[BUTTONS]", buttons);
        homePageHtml.replace("[HOST_NAME]", hostName);
        homePageHtml.trim();
    }
};

const String ConfigItem::inputElementTemplate = R"=====(
    <label>[ALIAS]:
        <input name='[NAME]' [REQUIRED] type='text' value='[VALUE]' placeholder='[PLACEHOLDER]' title='[PLACEHOLDER]' pattern='[PATTERN]'>
    </label>
)=====";

const String Config::homepageButtonTemplate = R"=====(
    <button onclick="window.location.href='[NAME]Config'">[ALIAS]配置页面</button>
)=====";

const String Config::configPageHtmlTemplate = R"=====(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta content='width=device-width, initial-scale=1.0' name='viewport'>
    <title>[ALIAS]配置页面</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            padding: 0;
            background-color: #f0f0f0;
        }

        form {
            max-width: 300px;
            margin: auto;
            padding: 20px;
            background-color: #ffffff;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }

        h2 {
            text-align: center;
            color: #333;
        }

        input[type=text] {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 5px;
            box-sizing: border-box;
        }

        input[type=submit] {
            width: 100%;
            background-color: #4CAF50;
            color: white;
            padding: 14px 20px;
            margin: 8px 0;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }

        input[type=submit]:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
<h2>[ALIAS]配置</h2>
<form method='POST' name='input' onsubmit='return submitForm()'>
    [ITEMS]
    <input type='submit' value='保存'>
</form>
</body>
<script>
    function submitForm() {
        const formData = new FormData(document.forms['input']);
        fetch('/[NAME]Config', {method: 'POST', body: formData})
            .then(response => {
                console.log(response);
            })
        return false;
    }
</script>
</html>
)=====";

const String HomePage::homePageHtmlTemplate = R"=====(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta content='width=device-width, initial-scale=1.0' name='viewport'>
    <title>[HOST_NAME]主页</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            padding: 0;
            background-color: #f0f0f0;
        }

        .button-group {
            max-width: 300px;
            margin: auto;
            padding: 20px;
            background-color: #ffffff;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }

        h2 {
            text-align: center;
            color: #333;
        }

        .button-group button {
            width: 100%;
            background-color: #4CAF50;
            color: white;
            padding: 14px 20px;
            margin: 8px 0;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }

        .button-group button:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
<h2>[HOST_NAME]主页</h2>
<div class="button-group">
    [BUTTONS]
</div>
</body>
</html>
)=====";
String HomePage::homePageHtml;