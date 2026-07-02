#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

std::string levelKey(int levelID, const char* suffix);
float loadPercentForLevel(int levelID, const char* suffix, float defaultValue);
bool loadDisabledForLevel(int levelID, const char* suffix, bool defaultValue);
void reloadPlayLayerThresholds();

class ChatConfigPopup : public geode::Popup {
protected:
    geode::TextInput* m_textInput1 = nullptr;
    geode::TextInput* m_textInput2 = nullptr;
    geode::TextInput* m_textInput3 = nullptr;
    geode::TextInput* m_textInput4 = nullptr;
    geode::TextInput* m_textInput5 = nullptr;
    CCMenuItemToggler* m_enableToggle = nullptr;

    bool init(float width, float height);
    void onToggle(CCObject*);

public:
    static ChatConfigPopup* create();
    void onClose(CCObject* sender) override;
};