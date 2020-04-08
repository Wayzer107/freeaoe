/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

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

#pragma once

#include "IRenderTarget.h"

#include "resource/Graphic.h"

namespace sf {
class Drawable;
class Font;
class Image;
class RenderTarget;
class Sprite;
class Texture;
class Text;
class RenderWindow;
class Event;
}

struct SfmlImage : public Drawable::Image
{
    SfmlImage() {}

    bool isValid() const override {
        return texture != nullptr;
    }

    std::unique_ptr<sf::Texture> texture;
};

struct SfmlText : public Drawable::Text
{
    SfmlText();

    std::unique_ptr<sf::Text> text;
    ScreenPos lastPos;
    Alignment lastAlignment = AlignLeft;

    float lineSpacing() override;
    Size size() override;
};

struct SfmlWindow : public Window
{
    void resize(const Size newSize) override;
    Size size() override;
    std::shared_ptr<IRenderTarget> createRenderTarget() override;

    bool isOpen() const override;
    void close() override;
    void update() override;

    std::shared_ptr<Event> waitEvent() override;
    std::shared_ptr<Event> pollEvent() override;

    std::unique_ptr<sf::RenderWindow> window;

private:
    std::shared_ptr<Window::MouseEvent> createMouseEvent(const sf::Event &sfEvent);
    std::shared_ptr<Window::Event> convertEvent(const sf::Event &event);

    bool m_altPressed = false;
    bool m_ctrlPressed = false;
    bool m_shiftPressed = false;
    ScreenPos m_mousePos;
    MouseEvent::Button m_pressedMouseButton = MouseEvent::NoButton;
};

class SfmlRenderTarget : public IRenderTarget
{

public:
    static const sf::Font &defaultFont();
    static const sf::Font &stylishFont();

    SfmlRenderTarget(const Size &size);
    SfmlRenderTarget(sf::RenderTarget &render_target);
    virtual ~SfmlRenderTarget();

    //----------------------------------------------------------------------------
    Size getSize() const override;
    void setSize(const Size size) const override;

    //----------------------------------------------------------------------------
    void draw(const sf::Image &image, ScreenPos pos) override;

    //----------------------------------------------------------------------------
    void draw(const sf::Texture &texture, ScreenPos pos) override;

    //----------------------------------------------------------------------------
    void draw(const sf::Drawable &shape) override;
    void draw(const sf::Sprite &sprite) override;
    void draw(const sf::Sprite &sprite, const sf::BlendMode &blendMode) override;

    void draw(const ScreenRect &rect, const Drawable::Color &fillColor, const Drawable::Color &outlineColor = Drawable::Transparent, const float outlineSize = 1.) override;
    void draw(const std::shared_ptr<IRenderTarget> &renderTarget, const sf::BlendMode &blendMode) override;

    void draw(const Drawable::Rect &rect) override;
    void draw(const Drawable::Circle &circle) override;

    Drawable::Image::Ptr createImage(const Size &size, const uint8_t *bytes) override;
    void draw(const Drawable::Image::Ptr &image, const ScreenPos &position) override;
    void draw(const std::shared_ptr<IRenderTarget> &renderTarget, const ScreenPos &pos = ScreenPos(0, 0)) override;

    //----------------------------------------------------------------------------
    /// Displays frame.
    //
    void display() override;

    sf::RenderTarget *renderTarget_;

    std::shared_ptr<IRenderTarget> createTextureTarget(const Size &size) override;

    void clear(const Drawable::Color &color = Drawable::Color(0, 0, 0, 255)) override;

    std::unique_ptr<sf::RenderTexture> m_renderTexture;


    Drawable::Text::Ptr createText() override;
    void draw(const Drawable::Text::Ptr &text) override;
};

