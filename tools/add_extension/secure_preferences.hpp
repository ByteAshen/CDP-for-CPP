#pragma once
#include "sha256.hpp"
#include "hmac.hpp"
#include "json.hpp"
#include "base64.hpp"
#include "platform.hpp"
#include "extension_id.hpp"
#include <set>
#include <iostream>

namespace chrome {

inline const std::vector<uint8_t> CHROME_SEED = {
    0xe7, 0x48, 0xf3, 0x36, 0xd8, 0x5e, 0xa5, 0xf9, 0xdc, 0xdf, 0x25, 0xd8, 0xf3, 0x47, 0xa6, 0x5b,
    0x4c, 0xdf, 0x66, 0x76, 0x00, 0xf0, 0x2d, 0xf6, 0x72, 0x4a, 0x2a, 0xf1, 0x8a, 0x21, 0x2d, 0x26,
    0xb7, 0x88, 0xa2, 0x50, 0x86, 0x91, 0x0c, 0xf3, 0xa9, 0x03, 0x13, 0x69, 0x68, 0x71, 0xf3, 0xdc,
    0x05, 0x82, 0x37, 0x30, 0xc9, 0x1d, 0xf8, 0xba, 0x5c, 0x4f, 0xd9, 0xc8, 0x84, 0xb5, 0x05, 0xa8
};

inline std::string calc_hmac(const std::string& message, const std::string& sid, const std::string& path = "") {
    std::string data = sid + path + message;
    return crypto::HMAC_SHA256::compute_hex(CHROME_SEED, data);
}

inline json::Value remove_empty_entries(const json::Value& obj) {
    if (obj.is_object()) {
        json::Object result;
        for (const auto& [key, value] : obj.as_object()) {
            json::Value cleaned = remove_empty_entries(value);
            if (cleaned.is_object() && cleaned.as_object().empty()) continue;
            if (cleaned.is_array() && cleaned.as_array().empty()) continue;
            if (cleaned.is_string() && cleaned.as_string().empty()) continue;
            result[key] = std::move(cleaned);
        }
        return json::Value(std::move(result));
    } else if (obj.is_array()) {
        json::Array result;
        for (const auto& elem : obj.as_array()) {
            result.push_back(remove_empty_entries(elem));
        }
        return json::Value(std::move(result));
    }
    return obj;
}

inline std::string json_for_mac(const json::Value& obj) {
    return json::Serializer::serialize(obj, true);
}

inline json::Value parse_manifest(const std::string& path) {
    std::string content = platform::read_file(path);
    return json::Parser::parse(content);
}

inline std::vector<std::string> get_scriptable_hosts(const json::Value& manifest) {
    std::set<std::string> hosts;
    if (manifest.contains("content_scripts")) {
        for (const auto& cs : manifest["content_scripts"].as_array()) {
            if (cs.contains("matches")) {
                for (const auto& match : cs["matches"].as_array()) {
                    hosts.insert(match.as_string());
                }
            }
        }
    }
    return std::vector<std::string>(hosts.begin(), hosts.end());
}

inline std::vector<std::string> get_api_permissions(const json::Value& manifest) {
    std::set<std::string> perms;
    if (manifest.contains("permissions")) {
        for (const auto& perm : manifest["permissions"].as_array()) {
            perms.insert(perm.as_string());
        }
    }
    return std::vector<std::string>(perms.begin(), perms.end());
}

inline std::vector<std::string> get_explicit_hosts(const json::Value& manifest) {
    std::set<std::string> hosts;
    if (manifest.contains("host_permissions")) {
        for (const auto& host : manifest["host_permissions"].as_array()) {
            hosts.insert(host.as_string());
        }
    }
    return std::vector<std::string>(hosts.begin(), hosts.end());
}

inline json::Value build_extension_entry(
    const std::string& ext_path,
    const json::Value& manifest,
    bool incognito,
    bool file_access
) {
    auto scriptable_hosts = get_scriptable_hosts(manifest);
    auto api_permissions = get_api_permissions(manifest);
    auto explicit_hosts = get_explicit_hosts(manifest);
    std::string version = manifest.get<std::string>("version", "1.0");
    std::string now = platform::chrome_time_now();

    json::Array api_arr, explicit_arr, scriptable_arr;
    for (const auto& p : api_permissions) api_arr.push_back(json::Value(p));
    for (const auto& h : explicit_hosts) explicit_arr.push_back(json::Value(h));
    for (const auto& h : scriptable_hosts) scriptable_arr.push_back(json::Value(h));

    json::Object active_permissions;
    active_permissions["api"] = json::Value(api_arr);
    active_permissions["explicit_host"] = json::Value(explicit_arr);
    active_permissions["manifest_permissions"] = json::Value(json::Array{});
    active_permissions["scriptable_host"] = json::Value(scriptable_arr);

    json::Object granted_permissions;
    granted_permissions["api"] = json::Value(api_arr);
    granted_permissions["explicit_host"] = json::Value(explicit_arr);
    granted_permissions["manifest_permissions"] = json::Value(json::Array{});
    granted_permissions["scriptable_host"] = json::Value(scriptable_arr);

    json::Object sw_info;
    sw_info["version"] = json::Value(version);

    json::Object entry;
    entry["account_extension_type"] = json::Value(0);
    entry["active_permissions"] = json::Value(std::move(active_permissions));
    entry["commands"] = json::Value(json::Object{});
    entry["content_settings"] = json::Value(json::Array{});
    entry["creation_flags"] = json::Value(38);
    entry["disable_reasons"] = json::Value(json::Array{});
    entry["first_install_time"] = json::Value(now);
    entry["from_webstore"] = json::Value(false);
    entry["granted_permissions"] = json::Value(std::move(granted_permissions));
    entry["incognito_content_settings"] = json::Value(json::Array{});
    entry["incognito_preferences"] = json::Value(json::Object{});
    entry["last_update_time"] = json::Value(now);
    entry["location"] = json::Value(4);
    entry["path"] = json::Value(ext_path);
    entry["preferences"] = json::Value(json::Object{});
    entry["regular_only_preferences"] = json::Value(json::Object{});
    entry["service_worker_registration_info"] = json::Value(std::move(sw_info));
    entry["serviceworkerevents"] = json::Value(json::Array{});
    entry["was_installed_by_default"] = json::Value(false);
    entry["was_installed_by_oem"] = json::Value(false);
    entry["withholding_permissions"] = json::Value(false);

    if (incognito) {
        entry["incognito"] = json::Value(true);
    }
    if (file_access) {
        entry["newAllowFileAccess"] = json::Value(true);
    }

    return json::Value(std::move(entry));
}

inline void create_secure_preferences(
    const std::string& user_data_dir,
    const std::vector<std::string>& extension_paths,
    bool incognito = true,
    bool file_access = true
) {
    namespace fs = std::filesystem;

    fs::path user_data_path(user_data_dir);
    fs::path default_path = user_data_path / "Default";
    platform::create_directories(default_path.string());

    std::string sid = platform::get_sid();
    if (sid.empty()) {
        throw std::runtime_error("Failed to get SID");
    }
    std::cout << "SID: " << sid << "\n";

    json::Object ext_settings;
    json::Object ext_macs;

    for (const auto& extension_path : extension_paths) {
        std::string ext_path = platform::normalize_path(extension_path);

        fs::path manifest_path = fs::path(ext_path) / "manifest.json";
        json::Value manifest = parse_manifest(manifest_path.string());

        std::string ext_id;
        if (manifest.contains("key")) {
            ext_id = generate_extension_id_from_key(manifest["key"].as_string());
            std::cout << "\nExtension ID: " << ext_id << " (from key)\n";
        } else {
            ext_id = generate_extension_id(ext_path);
            std::cout << "\nExtension ID: " << ext_id << "\n";
        }
        std::cout << "  Path: " << ext_path << "\n";

        auto scriptable_hosts = get_scriptable_hosts(manifest);
        auto api_permissions = get_api_permissions(manifest);
        auto explicit_hosts = get_explicit_hosts(manifest);

        std::cout << "  Scriptable hosts: [";
        for (size_t i = 0; i < scriptable_hosts.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << scriptable_hosts[i];
        }
        std::cout << "]\n";

        std::cout << "  API permissions: [";
        for (size_t i = 0; i < api_permissions.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << api_permissions[i];
        }
        std::cout << "]\n";

        std::cout << "  Explicit hosts: [";
        for (size_t i = 0; i < explicit_hosts.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << explicit_hosts[i];
        }
        std::cout << "]\n";

        json::Value ext_entry = build_extension_entry(ext_path, manifest, incognito, file_access);

        if (incognito) std::cout << "  Incognito: enabled\n";
        if (file_access) std::cout << "  File Access: enabled\n";

        ext_settings[ext_id] = ext_entry;

        json::Value ext_for_mac = remove_empty_entries(ext_entry);
        std::string mac_json = json_for_mac(ext_for_mac);
        std::string mac_path = "extensions.settings." + ext_id;
        std::string ext_mac = calc_hmac(mac_json, sid, mac_path);
        ext_macs[ext_id] = json::Value(ext_mac);
        std::cout << "  MAC: " << ext_mac << "\n";
    }

    std::string dev_mac = calc_hmac("true", sid, "extensions.ui.developer_mode");
    std::cout << "\nDeveloper mode MAC: " << dev_mac << "\n";

    json::Object ui_macs;
    ui_macs["developer_mode"] = json::Value(dev_mac);

    json::Object extensions_macs;
    extensions_macs["settings"] = json::Value(std::move(ext_macs));
    extensions_macs["ui"] = json::Value(std::move(ui_macs));

    json::Object macs_obj;
    macs_obj["extensions"] = json::Value(std::move(extensions_macs));

    std::string macs_json = json::Serializer::serialize(json::Value(macs_obj), false);
    std::string super_mac = calc_hmac(macs_json, sid);
    std::cout << "Super MAC: " << super_mac << "\n";

    json::Object ui_settings;
    ui_settings["developer_mode"] = json::Value(true);

    json::Object extensions;
    extensions["settings"] = json::Value(std::move(ext_settings));
    extensions["ui"] = json::Value(std::move(ui_settings));

    json::Object protection;
    protection["macs"] = json::Value(std::move(macs_obj));
    protection["super_mac"] = json::Value(super_mac);

    json::Object preferences;
    preferences["extensions"] = json::Value(std::move(extensions));
    preferences["protection"] = json::Value(std::move(protection));

    fs::path secure_prefs_path = default_path / "Secure Preferences";
    std::string prefs_json = json::Serializer::serialize(json::Value(std::move(preferences)), false);
    platform::write_file(secure_prefs_path.string(), prefs_json);
    std::cout << "\nSecure Preferences written to: " << secure_prefs_path << "\n";

    fs::path prefs_path = default_path / "Preferences";
    platform::write_file(prefs_path.string(), "{}");

    json::Object default_info;
    default_info["name"] = json::Value("Default");

    json::Object info_cache;
    info_cache["Default"] = json::Value(std::move(default_info));

    json::Object profile;
    profile["info_cache"] = json::Value(std::move(info_cache));

    json::Object local_state;
    local_state["profile"] = json::Value(std::move(profile));

    fs::path local_state_path = user_data_path / "Local State";
    std::string local_state_json = json::Serializer::serialize(json::Value(std::move(local_state)), false);
    platform::write_file(local_state_path.string(), local_state_json);
    std::cout << "Local State written to: " << local_state_path << "\n";

    std::cout << "\nDone! Launch Chrome with:\n";
    std::cout << "  chrome.exe --user-data-dir=\"" << fs::absolute(user_data_path).string() << "\"\n";
}

}