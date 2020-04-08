/*
    Copyright (C) 2018 Martin Sandsmark <martin.sandsmark@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "HistoryScreen.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Glyph.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <genie/resource/SlpFile.h>
#include <genie/resource/SlpFrame.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_set>
#include <utility>

#include "core/Logger.h"
#include "core/Utility.h"
#include "render/SfmlRenderTarget.h"
#include "resource/AssetManager.h"
#include "resource/LanguageManager.h"
#include "resource/Resource.h"

HistoryScreen::HistoryScreen() :
    UiScreen("scr_hist.sin")
{
}

bool HistoryScreen::init(const std::string &filesDir)
{
    if (!std::filesystem::exists(filesDir)) {
        WARN << filesDir << "does not exist";
        return false;
    }
    if (!UiScreen::init()) {
        return false;
    }

    std::shared_ptr<genie::SlpFile> slpFile = AssetManager::Inst()->getSlp("btn_hist.slp", AssetManager::ResourceType::Interface);
    if (!slpFile) {
        WARN << "failed to load buttons";
        return false;
    }

    if (slpFile->getFrameCount() < 7) {
        WARN << "not enough frames in button SLP" << slpFile->getFrameCount();
        return false;
    }

    const genie::PalFile &palette = AssetManager::Inst()->getPalette(m_paletteId);

    for (int i=0; i<UiElementsCount; i++) {
        UiElement &element = m_uiElements[i];

        int frameNum = -1;
        int hlFramenum = -1;
        switch(i) {
        case TitlesScrollbar:
            element.rect.x = 217;
            element.rect.y = 30;
            frameNum = LargeScrollbarTexture;
            break;
        case TitlesPositionIndicator:
            element.rect.x = 210;
            element.rect.y = 40;
            frameNum = ScrollPositionTexture;
            break;
        case TitlesUpButton:
            element.rect.x = 212;
            element.rect.y = 25;
            hlFramenum = ActiveUpButtonTexture;
            frameNum = UpButtonTexture;
            break;
        case TitlesDownButton:
            element.rect.x = 212;
            element.rect.y = 369;
            hlFramenum = ActiveDownButtonTexture;
            frameNum = DownButtonTexture;
            break;

        case TextScrolllbar:
            element.rect.x = 735;
            element.rect.y = 286;
            frameNum = SmallScrollbarTexture;
            break;
        case TextPositionIndicator:
            element.rect.x = 728;
            element.rect.y = 286;
            frameNum = ScrollPositionTexture;
            break;
        case TextUpButton:
            element.rect.x = 730;
            element.rect.y = 271;
            frameNum = UpButtonTexture;
            hlFramenum = ActiveUpButtonTexture;
            break;

        case TextDownButton:
            element.rect.x = 730;
            element.rect.y = 510;
            frameNum = DownButtonTexture;
            hlFramenum = ActiveDownButtonTexture;
            break;
        case MainScreenButton:
            continue;
        default:
            frameNum = 0;
            break;
        }

        if (hlFramenum != -1) {
            element.hoverTexture.loadFromImage(Resource::convertFrameToImage(slpFile->getFrame(hlFramenum), palette));
        }

        const genie::SlpFramePtr &frame = slpFile->getFrame(frameNum);
        element.texture.loadFromImage(Resource::convertFrameToImage(frame, palette));
        element.rect.width = frame->getWidth();
        element.rect.height = frame->getHeight();
    }

    slpFile = AssetManager::Inst()->getSlp("hist_pic.sin", AssetManager::ResourceType::Interface);

    int numEntries = std::stoi(LanguageManager::getString(20310));

    // For some reason there's duplicates, with blank illustrations, so we need to skip these
    std::unordered_set<std::string> addedItems;
    for (int i=0; i<numEntries; i++) {
        const std::string title = LanguageManager::getString(20310 + 1 + i);
        if (addedItems.count(title)) {
            continue;
        }

        HistoryEntry entry;
        entry.illustration.loadFromImage(Resource::convertFrameToImage(slpFile->getFrame(i), palette));
        entry.title = title;
        entry.index = i;

        std::string compareFilename = util::toLowercase(LanguageManager::getString(20410 + 1 + i));

        std::string filePath;
        for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(filesDir)) {
            std::string candidate = util::toLowercase(entry.path().filename().string());
            if (candidate == compareFilename) {
                filePath = entry.path().string();
                break;
            }
        }

        if (filePath.empty()) {
            WARN << "failed to find" << compareFilename;
        }
        entry.filename = filePath;

        m_historyEntries.push_back(entry);
        addedItems.insert(title);
    }

    slpFile = AssetManager::Inst()->getSlp(50768, AssetManager::ResourceType::Interface);
    if (!slpFile) {
        WARN << "Failed to load button bg";
        return false;
    }

    const sf::Font &stylishFont = SfmlRenderTarget::stylishFont();

    float posY = 30;
    for (int i=0; i<s_numListEntries; i++) {
        m_visibleTitles[i].text.setFont(stylishFont);
        m_visibleTitles[i].text.setCharacterSize(s_titlesTextSize);
        m_visibleTitles[i].text.setPosition(17, posY);
        m_visibleTitles[i].text.setFillColor(sf::Color::Black);
        m_visibleTitles[i].text.setOutlineThickness(1.5);
        m_visibleTitles[i].text.setOutlineColor(sf::Color::Transparent);
        m_visibleTitles[i].rect = ScreenRect(17, posY, 195, stylishFont.getLineSpacing(s_titlesTextSize));
        posY += stylishFont.getLineSpacing(s_titlesTextSize) * 1.2;
    }

    m_textRect.x = 317;
    m_textRect.y = 275;
    m_textRect.width = s_textWidth;
    m_textRect.height = 255;

    const sf::Font &font = SfmlRenderTarget::defaultFont();
    posY = m_textRect.y;
    for (int i=0; i<s_numVisibleTextLines; i++) {
        m_visibleText[i].setFont(font);
        m_visibleText[i].setCharacterSize(s_mainTextSize);
        m_visibleText[i].setPosition(m_textRect.x, posY);
        m_visibleText[i].setFillColor(sf::Color::Black);
        posY += font.getLineSpacing(s_mainTextSize);
    }

    // Main screen button
    const genie::PalFile &buttonPalette = AssetManager::Inst()->getPalette(50531);
    genie::SlpFramePtr buttonBg = slpFile->getFrame(0);
    m_uiElements[MainScreenButton].texture.loadFromImage(Resource::convertFrameToImage(buttonBg, buttonPalette));
    m_uiElements[MainScreenButton].pressTexture.loadFromImage(Resource::convertFrameToImage(slpFile->getFrame(1), buttonPalette));
    const ScreenRect buttonRect(m_textRect.center().x - buttonBg->getWidth() / 2,  m_textRect.bottom(), buttonBg->getWidth(), buttonBg->getHeight());
    m_uiElements[MainScreenButton].rect = buttonRect;

    m_mainScreenText.setFont(SfmlRenderTarget::stylishFont());
    m_mainScreenText.setString("Main Menu");
    m_mainScreenText.setCharacterSize(s_buttonTextSize);
    m_mainScreenText.setFillColor(m_textFillColor);
    m_mainScreenText.setOutlineColor(m_textOutlineColor);
    m_mainScreenText.setOutlineThickness(1);

    loadFile(m_historyEntries[0].filename);

    updateVisibleTitles();

    return true;
}

void HistoryScreen::display()
{
    run();
}


void HistoryScreen::render()
{
    sf::Sprite sprite;
    const sf::Texture &illustration = m_historyEntries[m_currentEntry].illustration;
    sprite.setPosition(525 - illustration.getSize().x/2, 70);
    sprite.setTexture(illustration);
    m_renderTarget->draw(sprite);

    for (int i=0; i<UiElementsCount; i++) {
        sf::Sprite sprite;
        if (i == m_pressedUiElement && m_uiElements[i].pressTexture.getSize().x > 0) {
            sprite.setTexture(m_uiElements[i].pressTexture);
        } else if (i == m_currentUiElement && m_uiElements[i].hoverTexture.getSize().x > 0) {
            sprite.setTexture(m_uiElements[i].hoverTexture);
        } else {
            sprite.setTexture(m_uiElements[i].texture);
        }
        sprite.setPosition(m_uiElements[i].rect.topLeft());
        m_renderTarget->draw(sprite);
    }

    for (int i=0; i<s_numVisibleTextLines; i++) {
        m_renderTarget->draw(m_visibleText[i]);
    }
    for (int i=0; i<s_numListEntries; i++) {
        m_renderTarget->draw(m_visibleTitles[i].text);
    }

    int textX = m_uiElements[MainScreenButton].rect.center().x - m_mainScreenText.getLocalBounds().width / 2;
    int textY = m_uiElements[MainScreenButton].rect.center().y - m_mainScreenText.getLocalBounds().height / 2;
    if (m_pressedUiElement == MainScreenButton) {
        textX += 2;
        textY -= 2;
    }
    m_mainScreenText.setPosition(textX, textY);
    m_renderTarget->draw(m_mainScreenText);
}

bool HistoryScreen::handleMouseEvent(const Window::MouseEvent::Ptr &event)
{
    if (event->type == Window::Event::MouseMoved) {
        if (m_pressedUiElement == TitlesPositionIndicator) {
            const int maxY = m_uiElements[TitlesDownButton].rect.y - m_uiElements[TitlesUpButton].rect.bottom() - m_uiElements[TitlesPositionIndicator].rect.height/2;
            m_titleScrollOffset = (m_historyEntries.size() - s_numListEntries) * (event->position.y - m_uiElements[TitlesUpButton].rect.bottom()) / maxY;
            m_titleScrollOffset = std::min(m_titleScrollOffset, int(m_historyEntries.size()) - s_numListEntries);
            m_titleScrollOffset = std::max(m_titleScrollOffset, 0);
            updateVisibleTitles();
            return false;
        }
        if (m_pressedUiElement == TextPositionIndicator) {
            const int maxY = m_uiElements[TextDownButton].rect.y - m_uiElements[TextUpButton].rect.bottom() - m_uiElements[TextPositionIndicator].rect.height/2;
            m_textScrollOffset = (m_textLines.size() - s_numListEntries) * (event->position.y - m_uiElements[TextUpButton].rect.bottom()) / maxY;
            m_textScrollOffset = std::min(m_textScrollOffset, int(m_textLines.size()) - s_numVisibleTextLines);
            m_textScrollOffset = std::max(m_textScrollOffset, 0);
            updateVisibleText();
            return false;
        }
        m_currentUiElement = InvalidUiElement;
        for (int i=UiElementsCount-1; i>=0; i--) {
            if (m_uiElements[i].rect.contains(ScreenPos(event->position.x, event->position.y)) && m_uiElements[i].hoverTexture.getSize().x > 0) {
                m_currentUiElement = UiElements(i);
                return false;
            }
        }
        return false;
    }

    if (event->type == Window::Event::MousePressed) {
        m_pressedUiElement = InvalidUiElement;

        for (int i=0; i<s_numListEntries; i++) {
            if (m_visibleTitles[i].rect.contains(ScreenPos(event->position.x, event->position.y))) {
                const int index = i + m_titleScrollOffset;
                m_currentEntry = index;
                loadFile(m_historyEntries[index].filename);
                updateVisibleTitles();
                return false;
            }
        }

        for (int i=UiElementsCount-1; i>=0; i--) {
            if (!m_uiElements[i].rect.contains(ScreenPos(event->position.x, event->position.y))) {
                continue;
            }

            m_pressedUiElement = UiElements(i);

            switch (i) {
            case TitlesUpButton:
                if (m_titleScrollOffset > 0) {
                    m_titleScrollOffset--;
                    updateVisibleTitles();
                }

                break;
            case TitlesDownButton:
                if (m_titleScrollOffset < int(m_historyEntries.size()) - s_numListEntries) {
                    m_titleScrollOffset++;
                    updateVisibleTitles();
                }

                break;
            case TitlesScrollbar: {
                const int maxY = m_uiElements[TitlesDownButton].rect.y - m_uiElements[TitlesUpButton].rect.bottom() - m_uiElements[TitlesPositionIndicator].rect.height/2;
                m_titleScrollOffset = (m_historyEntries.size() - s_numListEntries) * (event->position.y - m_uiElements[TitlesUpButton].rect.bottom()) / maxY;
                updateVisibleTitles();
                m_pressedUiElement = TitlesPositionIndicator;
                break;
            }
            case TextUpButton:
                if (m_textScrollOffset > 0) {
                    m_textScrollOffset--;
                    updateVisibleText();
                }
                break;
            case TextDownButton:
                if (m_textScrollOffset < int(m_textLines.size()) - s_numVisibleTextLines) {
                    m_textScrollOffset++;
                    updateVisibleText();
                }
                break;
            case TextScrolllbar: {
                const int maxY = m_uiElements[TextDownButton].rect.y - m_uiElements[TextUpButton].rect.bottom() - m_uiElements[TextPositionIndicator].rect.height/2;
                m_textScrollOffset = (m_textLines.size() - s_numListEntries) * (event->position.y - m_uiElements[TextUpButton].rect.bottom()) / maxY;
                updateVisibleText();
                m_pressedUiElement = TextPositionIndicator;
                break;
            }
            default:
                continue;
            }


            return false;
        }
        return false;
    }

    if (event->type == Window::Event::MouseReleased) {
        if (m_pressedUiElement == MainScreenButton && m_uiElements[m_pressedUiElement].rect.contains(event->position)) {
            return true;
        }

        m_pressedUiElement = InvalidUiElement;
        return false;
    }

    return false;
}

void HistoryScreen::handleKeyEvent(const Window::KeyEvent::Ptr &event)
{
    if (event->key == Window::KeyEvent::Up) {
        if (m_textScrollOffset > 0) {
            m_textScrollOffset--;
            updateVisibleText();
        }
    } else if (event->key == Window::KeyEvent::Down) {
        if (m_textScrollOffset < int(m_textLines.size()) - s_numVisibleTextLines) {
            m_textScrollOffset++;
            updateVisibleText();
        }
    }

}

void HistoryScreen::handleScrollEvent(const Window::MouseScrollEvent::Ptr &event)
{
    if (event->position.x > 22 && event->position.x < 220 && event->position.y > 25 && event->position.y < 375) {
        if (event->deltaY < 0) {
            if (m_titleScrollOffset < int(m_historyEntries.size()) - s_numListEntries) {
                m_titleScrollOffset++;
                updateVisibleTitles();
            }
        } else {
            if (m_titleScrollOffset > 0) {
                m_titleScrollOffset--;
                updateVisibleTitles();
            }
        }
    }
    if (m_textRect.contains(event->position)) {
        if (event->deltaY < 0) {
            if (m_textScrollOffset < int(m_textLines.size()) - s_numVisibleTextLines) {
                m_textScrollOffset++;
                updateVisibleText();
            }
        } else {
            if (m_textScrollOffset > 0) {
                m_textScrollOffset--;
                updateVisibleText();
            }
        }
    }
}

void HistoryScreen::loadFile(const std::string &filePath)
{
    m_textLines.clear();

    std::ifstream file(filePath, std::ios_base::binary);
    if (!file.is_open()) {
        WARN << "failed to open" << filePath;
        return;
    }

    m_textScrollOffset = 0;

    TextLine currentLine;

    const sf::Font &font = SfmlRenderTarget::defaultFont();

    std::string currentWord;
    float currentWordWidth= 0.f;
    const float spaceWidth = font.getGlyph(' ', s_mainTextSize, false).advance;
    while (!file.eof()) {
        char character = file.get();

        if (character == '\r') {
            continue;
        }

        if (character != ' ' && character != '\n') {
            currentWord += character;
            currentWordWidth += font.getGlyph(character, s_mainTextSize, false).advance;
            continue;
        }

        if (character == '\n') {
            if (currentLine.width + currentWordWidth > s_textWidth) {
                m_textLines.push_back(std::move(currentLine));
            }

            currentLine.text += currentWord;

            m_textLines.push_back(std::move(currentLine));
            currentLine.width = 0;
            currentWord.clear();
            currentWordWidth = 0;
            continue;
        }

        if (currentLine.width + currentWordWidth > s_textWidth) {
            m_textLines.push_back(std::move(currentLine));
            currentLine.width = 0;
        }

        currentLine.text += currentWord + ' ';
        currentLine.width += currentWordWidth + spaceWidth;

        currentWord.clear();
        currentWordWidth = 0;
    }

    if (!currentLine.text.empty()) {
        m_textLines.push_back(std::move(currentLine));
    }

    // TODO: I'm lazy, parse good, different styling in a single line
    for (TextLine &line : m_textLines) {
        if (line.text.empty()) {
            continue;
        }
        if (line.text[0] != '<') {
            continue;
        }
        if (util::stringStartsWith(util::toLowercase(line.text), "<b>")) {
            line.bold = true;
        } else if (util::stringStartsWith(util::toLowercase(line.text), "<i>")) {
            line.italic = true;
        }
        line.text = util::stringReplace(line.text, "<b>", "");
        line.text = util::stringReplace(line.text, "<i>", "");
        line.text = util::stringReplace(line.text, "<B>", "");
        line.text = util::stringReplace(line.text, "<I>", "");
    }

    updateVisibleText();
}

void HistoryScreen::updateVisibleText()
{
    for (int i=0; i<s_numVisibleTextLines; i++) {
        m_visibleText[i].setStyle(sf::Text::Regular);

        const int index = i + m_textScrollOffset;
        if (index >= m_textLines.size()) {
            m_visibleText[i].setString("");
            continue;
        }

        m_visibleText[i].setString(m_textLines[index].text);

        if (m_textLines[index].bold) {
            m_visibleText[i].setStyle(m_visibleText[i].getStyle() | sf::Text::Bold);
        }

        if (m_textLines[index].italic) {
            m_visibleText[i].setStyle(m_visibleText[i].getStyle() | sf::Text::Italic);
        }
    }

    int maxY = m_uiElements[TextDownButton].rect.y - m_uiElements[TextUpButton].rect.bottom() - m_uiElements[TextPositionIndicator].rect.height;
    m_uiElements[TextPositionIndicator].rect.y = m_uiElements[TextUpButton].rect.bottom() + maxY * m_textScrollOffset / int(m_textLines.size() - s_numVisibleTextLines);
}

void HistoryScreen::updateVisibleTitles()
{
    for (int i=0; i<s_numListEntries; i++) {
        const int index = i + m_titleScrollOffset;

        if (index == m_currentEntry) {
            m_visibleTitles[i].text.setOutlineColor(sf::Color(192, 192, 0));
        } else {
            m_visibleTitles[i].text.setOutlineColor(sf::Color::Transparent);
        }

        if (index >= m_historyEntries.size()) {
            m_visibleTitles[i].text.setString("");
            continue;
        }
        m_visibleTitles[i].text.setString(m_historyEntries[index].title);
    }
    int maxY = m_uiElements[TitlesDownButton].rect.y - m_uiElements[TitlesUpButton].rect.bottom() - m_uiElements[TitlesPositionIndicator].rect.height;
    m_uiElements[TitlesPositionIndicator].rect.y = m_uiElements[TitlesUpButton].rect.bottom() + maxY * m_titleScrollOffset / int(m_historyEntries.size() - s_numListEntries);
}

