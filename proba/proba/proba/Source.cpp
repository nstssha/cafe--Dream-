#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <ctime>
#include <cstdlib>
#include <codecvt>
#include <algorithm>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace sf;
using namespace std;

// Функция преобразования строки в широкую строку (для поддержки русского текста)
wstring stringToWstring(const string& str) {
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

// Класс для поэтапного показа текста (эффект "печатной машинки")
class AnimatedText {
public:
    wstring fullText;         // Полный текст, который нужно отобразить
    wstring currentText;      // Текст, который уже отображён
    float duration;           // Интервал между символами
    float elapsedTime;        // Время с момента последнего добавления символа
    bool completed;           // Флаг, завершён ли показ текста
    Text displayText;         // Объект текста SFML для отображения

    AnimatedText() : duration(0.05f), elapsedTime(0.0f), completed(false) {}

    // Устанавливает новый текст и сбрасывает состояние
    void setString(const wstring& text) {
        fullText = text;
        currentText = L"";
        completed = false;
        elapsedTime = 0.0f;
        displayText.setString(currentText);
    }

    // Обновляет состояние анимации текста на экране
    void update(float dt) {
        if (completed) return;
        elapsedTime += dt;
        if (elapsedTime >= duration) {
            elapsedTime = 0.0f;
            if (currentText.length() < fullText.length()) {
                currentText = fullText.substr(0, currentText.length() + 1);
                displayText.setString(currentText);
            }
            else {
                completed = true;
            }
        }
    }
};

// Облако для отображения диалогов персонажей с анимацией текста
class DialogCloud {
public:
    Texture cloudTexture;       // Текстура облака
    Sprite cloudSprite;         // Спрайт облака

    AnimatedText dialogText;    // Анимированный текст
    Font font;                  // Шрифт для текста
    int currentPhrase;          // Текущий индекс фразы
    vector<wstring> phrases;    // Все фразы диалога

    bool visible;               // Видимость облака
    bool completed;             // Закончился ли диалог
    int clicksNeeded;           // Сколько раз нужно кликнуть
    int clickCount;             // Сколько раз уже кликнули

    Vector2f textOffset;        // Смещение текста внутри облака
    float maxTextWidth;         // Максимальная ширина строки

    DialogCloud()
        : currentPhrase(0), visible(false), completed(false),
        clicksNeeded(2), clickCount(0), textOffset(120, 50), maxTextWidth(200) {

        // Загрузка текстуры облака
        if (!cloudTexture.loadFromFile("Dialog/cloud.png")) {
            cout << "Ошибка загрузки облака диалога" << endl;
            cloudTexture.create(200, 100);
        }
        cloudSprite.setTexture(cloudTexture);

        // Загрузка шрифта
        if (!font.loadFromFile("Montserrat-SemiBold.ttf")) {
            cout << "Ошибка загрузки шрифта" << endl;
        }

        // Настройка параметров текста
        dialogText.displayText.setFont(font);
        dialogText.displayText.setCharacterSize(20);
        dialogText.displayText.setFillColor(Color::Black);
        dialogText.displayText.setPosition(textOffset);
    }

    // Запуск диалога с набором фраз
    void start(const vector<wstring>& dialogPhrases) {
        phrases = dialogPhrases;
        currentPhrase = 0;
        visible = true;
        completed = false;
        clickCount = 0;

        if (!phrases.empty()) {
            wstring wrappedText = wrapText(phrases[0], maxTextWidth, font, 20);
            dialogText.setString(wrappedText);
        }
    }

    // Перенос строк, чтобы текст влезал в облако
    wstring wrapText(const wstring& text, float maxWidth, const Font& font, unsigned int characterSize) {
        wstring result, currentLine, currentWord;

        for (wchar_t c : text) {
            if (c == L' ' || c == L'-' || c == L',') {
                Text testLine((currentLine + currentWord + c).c_str(), font, characterSize);
                if (testLine.getLocalBounds().width <= maxWidth || currentLine.empty())
                    currentLine += currentWord + c;
                else {
                    result += currentLine + L'\n';
                    currentLine = currentWord + c;
                }
                currentWord.clear();
            }
            else {
                currentWord += c;
            }
        }

        if (!currentWord.empty()) {
            Text testLine(currentLine + currentWord, font, characterSize);
            if (testLine.getLocalBounds().width <= maxWidth || currentLine.empty())
                currentLine += currentWord;
            else {
                result += currentLine + L'\n';
                currentLine = currentWord;
            }
        }

        result += currentLine;
        return result;
    }

    // Переход к следующей фразе
    void nextPhrase() {
        currentPhrase++;
        if (currentPhrase < phrases.size()) {
            wstring wrappedText = wrapText(phrases[currentPhrase], maxTextWidth, font, 20);
            dialogText.setString(wrappedText);
        }
        else {
            completed = true;
            visible = false;
        }
    }

    // Обработка клика: переходит к следующей фразе
    void handleClick() {
        if (!visible) return;
        if (dialogText.completed) {
            nextPhrase();
            clickCount++;
        }
    }

    // Обновляет анимацию текста
    void update(float dt) {
        if (!visible) return;
        dialogText.update(dt);
    }

    // Позиция облака на экране
    void setPosition(Vector2f pos) {
        cloudSprite.setPosition(pos);
        dialogText.displayText.setPosition(pos.x + textOffset.x, pos.y + textOffset.y);
    }

    // Проверка попадания клика
    bool contains(Vector2f point) {
        return cloudSprite.getGlobalBounds().contains(point);
    }

    // Отрисовка облака с текстом
    void draw(RenderWindow& window) {
        if (!visible) return;
        window.draw(cloudSprite);
        window.draw(dialogText.displayText);
    }
};


// Класс отображения заказов клиента
class OrderWindow {
public:
    Texture windowTexture;         // Текстура окна заказа
    Sprite windowSprite;           // Спрайт окна заказа
    vector<Sprite> itemSprites;    // Спрайты иконок заказанных блюд
    bool visible;                  // Видимость окна заказа
    vector<string> currentOrders;  // Список текущих блюд
    vector<string> servedOrders;   // Уже отданные блюда
    Vector2f position;             // Позиция окна на экране

    static map<string, Texture> preloadedTextures; // Предзагруженные текстуры всех возможных заказов

    OrderWindow() : visible(false) {
        // Загрузка текстуры окна
        if (!windowTexture.loadFromFile("Order/window.png")) {
            cout << "Ошибка загрузки окна заказа" << endl;
            windowTexture.create(250, 150);
        }
        windowSprite.setTexture(windowTexture);

        // Предзагрузка иконок всех типов заказов
        if (preloadedTextures.empty()) {
            preloadAllTextures();
        }
    }

    // Предзагружает изображения всех вариантов заказов (один раз для всего проекта)
    static void preloadAllTextures() {
        vector<string> allItems = {
            "coffee",
            "croissant_plain", "croissant_chocolate", "croissant_banana", "croissant_both",
            "sandwich_plain", "sandwich_sausage", "sandwich_cheese", "sandwich_pepper",
            "sandwich_sausage_cheese", "sandwich_sausage_pepper", "sandwich_cheese_pepper", "sandwich_all",
            "cake1", "cake2"
        };

        for (const auto& item : allItems) {
            Texture texture;
            if (!texture.loadFromFile("Order/" + item + ".png")) {
                // Заглушка в случае ошибки
                texture.create(65, 52);
                Uint8* pixels = new Uint8[65 * 52 * 4];
                for (int i = 0; i < 65 * 52 * 4; i += 4) {
                    pixels[i] = 255;     // Красный фон
                    pixels[i + 1] = 0;
                    pixels[i + 2] = 0;
                    pixels[i + 3] = 255;
                }
                texture.update(pixels);
                delete[] pixels;
            }
            preloadedTextures[item] = texture;
        }

        cout << "Предзагружено " << preloadedTextures.size() << " текстур заказов" << endl;
    }

    // Обновляет список отданных блюд и перерисовывает окно
    void setServedOrders(const vector<string>& served) {
        servedOrders = served;
        updateDisplay();
    }

    // Задаёт новый заказ и сбрасывает уже отданные блюда
    void setOrders(const vector<string>& orders) {
        currentOrders = orders;
        servedOrders.clear();
        updateDisplay();
    }

private:
    // Обновляет отображение блюд в окне
    void updateDisplay() {
        itemSprites.clear();

        // Исключаем уже отданные блюда из отображения
        vector<string> remainingOrders = currentOrders;
        for (const string& dish : servedOrders) {
            auto it = find(remainingOrders.begin(), remainingOrders.end(), dish);
            if (it != remainingOrders.end()) {
                remainingOrders.erase(it);
            }
        }

        // Отображаем максимум 4 блюда
        size_t count = min(remainingOrders.size(), static_cast<size_t>(4));

        for (size_t i = 0; i < count; i++) {
            const string& orderName = remainingOrders[i];
            auto it = preloadedTextures.find(orderName);
            if (it != preloadedTextures.end()) {
                Sprite sprite(it->second);
                sprite.setScale(72.5f / sprite.getLocalBounds().width,
                    56.f / sprite.getLocalBounds().height);
                itemSprites.push_back(sprite);
            }
            else {
                // Заглушка
                cout << "Текстура для " << orderName << " не найдена" << endl;
                Texture dummy;
                dummy.create(65, 52);
                Sprite sprite(dummy);
                sprite.setScale(72.5f / 65, 56.f / 52);
                itemSprites.push_back(sprite);
            }
        }

        updatePositions();
    }

public:
    // Установка позиции окна и пересчёт позиций иконок
    void setPosition(Vector2f pos) {
        position = pos;
        windowSprite.setPosition(pos);
        updatePositions();
    }

    // Вычисляет позиции иконок блюд внутри окна
    void updatePositions() {
        int count = itemSprites.size();
        if (count == 0) return;

        const float itemWidth = 72.5;
        const float itemHeight = 56;
        const float spacing = 15;

        // Центрирование
        float startX = position.x + (windowTexture.getSize().x -
            ((count > 2) ? 2 * itemWidth + spacing : count * itemWidth + (count - 1) * spacing)) / 2;
        float startY = position.y + (windowTexture.getSize().y -
            ((count > 2) ? 2 * itemHeight + spacing : itemHeight)) / 2;

        for (int i = 0; i < count; i++) {
            if (count <= 2) {
                itemSprites[i].setPosition(startX + i * (itemWidth + spacing), startY);
            }
            else {
                int row = i / 2;
                int col = i % 2;
                itemSprites[i].setPosition(startX + col * (itemWidth + spacing),
                    startY + row * (itemHeight + spacing));
            }
        }
    }

    // Отрисовка окна и блюд
    void draw(RenderWindow& window) {
        if (!visible) return;
        window.draw(windowSprite);
        for (auto& sprite : itemSprites) {
            window.draw(sprite);
        }
    }
};

// Статическое хранилище текстур заказов
map<string, Texture> OrderWindow::preloadedTextures;

// Класс управления сохранением прогресса игрока
class SaveManager {
private:
    string filename = "save.dat";   // Имя файла сохранения
    int maxUnlockedLevel;           // Максимально открытый уровень
    int currentLevel;               // Текущий выбранный уровень

public:
    // При запуске загружается сохранение
    SaveManager() : maxUnlockedLevel(1), currentLevel(1) {
        load();
    }

    // Загрузка прогресса из файла
    void load() {
        ifstream file(filename, ios::binary);
        if (file) {
            file.read(reinterpret_cast<char*>(&maxUnlockedLevel), sizeof(maxUnlockedLevel));
            file.read(reinterpret_cast<char*>(&currentLevel), sizeof(currentLevel));
        }
        else {
            maxUnlockedLevel = 1;
            currentLevel = 1;
            save(); // если файла нет, создаём дефолтное сохранение
        }
    }

    // Сохраняет текущий прогресс
    void save() {
        ofstream file(filename, ios::binary);
        if (file) {
            file.write(reinterpret_cast<char*>(&maxUnlockedLevel), sizeof(maxUnlockedLevel));
            file.write(reinterpret_cast<char*>(&currentLevel), sizeof(currentLevel));
        }
    }

    int getMaxUnlockedLevel() const { return maxUnlockedLevel; }
    int getCurrentLevel() const { return currentLevel; }

    void setCurrentLevel(int level) {
        if (level != currentLevel) {
            currentLevel = level;
            save();
        }
    }

    void unlockLevel(int level) {
        if (level > maxUnlockedLevel) {
            maxUnlockedLevel = level;
            save();
        }
    }

    // Полный сброс прогресса
    void resetProgress() {
        maxUnlockedLevel = 1;
        currentLevel = 1;
        save();
    }
};

// Класс заставки загрузки — проигрывает анимацию
class LoadingScreen {
public:
    Texture loadingTexture;   // Текстура, содержащая все кадры
    Sprite loadingSprite;     // Отображаемый спрайт

    int frameWidth;           // Ширина одного кадра
    int frameHeight;          // Высота одного кадра
    int columns;              // Кол-во колонок в текстуре
    int rows;                 // Кол-во строк в текстуре
    int totalFrames;          // Общее число кадров
    int currentFrame;         // Текущий отображаемый кадр
    float frameDuration;      // Длительность кадра
    float accumulatedTime;    // Время с начала текущего кадра
    bool completed;           // Анимация завершена

    LoadingScreen() : frameWidth(1280), frameHeight(720),
        columns(3), rows(3), totalFrames(columns* rows),
        currentFrame(0), frameDuration(0.17f),
        accumulatedTime(0.0f), completed(false)
    {
        // Загрузка текстуры заставки
        if (!loadingTexture.loadFromFile("Menu/loading.png")) {
            cout << "Ошибка загрузки заставки" << endl;
            loadingTexture.create(frameWidth, frameHeight);
        }

        loadingSprite.setTexture(loadingTexture);
        updateFrame();
    }

    // Обновляет отображаемый кадр
    void updateFrame() {
        int row = currentFrame / columns;
        int col = currentFrame % columns;
        IntRect frameRect(col * frameWidth, row * frameHeight, frameWidth, frameHeight);
        loadingSprite.setTextureRect(frameRect);
    }

    // Переходит к следующему кадру, если настало время
    void update(float dt) {
        if (completed) return;
        accumulatedTime += dt;
        while (accumulatedTime >= frameDuration) {
            accumulatedTime -= frameDuration;
            currentFrame++;
            if (currentFrame >= totalFrames) {
                completed = true;
                currentFrame = totalFrames - 1; // остаётся на последнем кадре
                break;
            }
            updateFrame();
        }
    }

    // Отрисовывает текущий кадр
    void draw(RenderWindow& window) {
        window.draw(loadingSprite);
    }
};


// Класс главного меню игры
class Menu {
public:
    Texture bgTexture;          // Фоновая анимация
    Sprite bgSprite;
    int frameWidth, frameHeight;
    int columns, rows;
    int totalFrames, currentFrame;
    float frameDuration, accumulatedTime;

    SoundBuffer clickBuffer;    // Звук нажатия
    Sound clickSound;

    struct MenuButton {
        Texture normalTex;      // Обычная текстура кнопки
        Texture hoverTex;       // При наведении
        Sprite sprite;
        float x, y;             // Координаты кнопки
        float width, height;
        float hoverY, hoverX;   // Смещение при наведении
        int action;             // Что делает кнопка (идентификатор)
        bool isHovered;
    };

    vector<MenuButton> buttons; // Все кнопки главного меню

    Texture aboutTexture;       // Изображение окна "о программе"
    Sprite aboutSprite;
    bool aboutVisible;

    bool* soundOn;              // Ссылка на глобальное состояние звука
    SaveManager* saveManager;   // Ссылка на менеджер сохранений
    bool prevSoundState;

    Texture soundOnTexture;     // Иконки звука
    Texture soundOffTexture;

    Menu(SaveManager* saveManager, bool* soundState)
        : saveManager(saveManager), soundOn(soundState), prevSoundState(*soundState),
        frameWidth(1280), frameHeight(720), columns(5), rows(4),
        totalFrames(rows* columns), currentFrame(0),
        frameDuration(0.1f), accumulatedTime(0.0f)
    {
        // Загрузка фоновой анимации
        if (!bgTexture.loadFromFile("Menu/menu.png")) {
            cout << "Ошибка загрузки фонового изображения меню" << endl;
            bgTexture.create(frameWidth, frameHeight);
        }
        bgSprite.setTexture(bgTexture);

        // Звук клика
        if (!clickBuffer.loadFromFile("audio/button_click.wav")) {
            cout << "Ошибка загрузки звука кнопки" << endl;
        }
        clickSound.setBuffer(clickBuffer);

        updateFrame();     // Устанавливаем первый кадр фона
        createButtons();   // Создаём кнопки
    }

    // Создаёт кнопки главного меню (Play, Continue, Exit, Sound, About)
    void createButtons() {
        buttons.clear();

        // Кнопки начала игры, продолжения и выхода
        createButton("Menu/play.png", "Menu/play_hover.png", 890, 200, 205, 900, 0);
        createButton("Menu/continue.png", "Menu/continue_hover.png", 890, 318, 322, 900, 1);
        createButton("Menu/exit.png", "Menu/exit_hover.png", 890, 438, 440, 900, 2);

        // Иконки звука
        if (!soundOnTexture.loadFromFile("Menu/sound_on.png"))
            cout << "Ошибка загрузки: sound_on.png" << endl;
        if (!soundOffTexture.loadFromFile("Menu/sound_off.png"))
            cout << "Ошибка загрузки: sound_off.png" << endl;

        createButton("Menu/sound_on.png", "Menu/sound_off.png", 1180, 20, 20, 1180, 3);  // кнопка звука
        createButton("Menu/about.png", "Menu/about_hover.png", 1100, 42, 45, 1103, 4);   // кнопка "о программе"

        updateSoundButton();  // Устанавливаем текущую иконку звука

        // Информация "о программе"
        if (!aboutTexture.loadFromFile("Menu/about_info.png")) {
            cout << "Ошибка загрузки информации о меню" << endl;
            aboutTexture.create(400, 300);
        }
        aboutSprite.setTexture(aboutTexture);
        FloatRect bounds = aboutSprite.getLocalBounds();
        aboutSprite.setOrigin(bounds.width / 2, bounds.height / 2);
        aboutSprite.setPosition(1280 / 2, 720 / 2);
        update(0, 0);
    }

    // Перезагрузка кнопок — вызывается при возврате из меню уровней
    void reload() {
        createButtons();
        update(0, 0);
    }

    // Обновляет текущий кадр фоновой анимации
    void updateFrame() {
        int row = currentFrame / columns;
        int col = currentFrame % columns;
        IntRect frameRect(col * frameWidth, row * frameHeight, frameWidth, frameHeight);
        bgSprite.setTextureRect(frameRect);
    }

    // Анимация фона — переключает кадры с течением времени
    void updateAnimation(float deltaTime) {
        if (totalFrames <= 1) return;
        accumulatedTime += deltaTime;
        if (accumulatedTime >= frameDuration) {
            accumulatedTime -= frameDuration;
            currentFrame = (currentFrame + 1) % totalFrames;
            updateFrame();
        }
    }

    // Создание отдельной кнопки
    void createButton(const string& normalFile, const string& hoverFile,
        float x, float y, float hoverY, float hoverX, int action)
    {
        MenuButton btn;
        btn.action = action;
        btn.x = x;
        btn.y = y;
        btn.hoverY = hoverY;
        btn.hoverX = hoverX;
        btn.isHovered = false;

        // Загружаем обычное изображение
        if (!btn.normalTex.loadFromFile(normalFile)) {
            cout << "Ошибка загрузки: " << normalFile << endl;
            btn.normalTex.create(50, 50); // Заглушка
            Uint8* pixels = new Uint8[50 * 50 * 4];
            for (int i = 0; i < 50 * 50 * 4; i += 4) {
                pixels[i] = 255; pixels[i + 1] = 0; pixels[i + 2] = 0; pixels[i + 3] = 255;
            }
            btn.normalTex.update(pixels);
            delete[] pixels;
        }

        btn.sprite.setTexture(btn.normalTex);

        // Загрузка картинки при наведении
        if (hoverFile.empty()) {
            btn.hoverTex = btn.normalTex;
        }
        else {
            if (!btn.hoverTex.loadFromFile(hoverFile)) {
                cout << "Ошибка загрузки hover: " << hoverFile << endl;
                btn.hoverTex = btn.normalTex;
            }
        }

        btn.sprite.setPosition(x, y);
        btn.width = static_cast<float>(btn.normalTex.getSize().x);
        btn.height = static_cast<float>(btn.normalTex.getSize().y);
        buttons.push_back(btn);
    }

    // Обновляет иконку кнопки звука в зависимости от текущего состояния
    void updateSoundButton() {
        for (auto& btn : buttons) {
            if (btn.action == 3) {
                if (*soundOn)
                    btn.sprite.setTexture(soundOnTexture);
                else
                    btn.sprite.setTexture(soundOffTexture);
            }
        }
    }

    // Проверка на смену состояния звука
    void updateSoundState() {
        if (*soundOn != prevSoundState) {
            updateSoundButton();
            prevSoundState = *soundOn;
        }
    }


    // Обновление состояния кнопок при движении мыши
    void update(float mouseX, float mouseY) {
        updateSoundState();

        for (auto& btn : buttons) {
            if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
                mouseY >= btn.y && mouseY <= btn.y + btn.height) {
                btn.isHovered = true;
                if (btn.action == 0 || btn.action == 1 || btn.action == 2 || btn.action == 4) {
                    btn.sprite.setTexture(btn.hoverTex);
                    btn.sprite.setPosition(btn.hoverX, btn.hoverY);
                }
            }
            else {
                btn.isHovered = false;
                if (btn.action == 0 || btn.action == 1 || btn.action == 2 || btn.action == 4) {
                    btn.sprite.setTexture(btn.normalTex);
                    btn.sprite.setPosition(btn.x, btn.y);
                }
            }
        }
    }

    // Обработка нажатия на кнопки
    int handleClick(float mouseX, float mouseY) {
        bool clickedOnButton = false;
        for (auto& btn : buttons) {
            if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
                mouseY >= btn.y && mouseY <= btn.y + btn.height) {
                clickedOnButton = true;

                if (btn.action != 3 && soundOn && *soundOn) {
                    clickSound.play();
                }

                // Возврат действия кнопки
                switch (btn.action) {
                case 0: return 0; // Play
                case 1: return 1; // Continue
                case 2: return 2; // Exit
                case 3: // Переключение звука
                    *soundOn = !*soundOn;
                    updateSoundButton();
                    prevSoundState = *soundOn;
                    return 3;
                case 4: // "О программе"
                    aboutVisible = !aboutVisible;
                    return 4;
                }
            }
        }

        // Клик вне кнопок — закрыть "о программе"
        if (!clickedOnButton)
            aboutVisible = false;

        return -1;
    }

    // Отрисовка меню: фон, кнопки, окно "о программе"
    void draw(RenderWindow& window) {
        window.draw(bgSprite);
        for (auto& btn : buttons) {
            window.draw(btn.sprite);
        }

        if (aboutVisible) {
            window.draw(aboutSprite);
        }
    }
};

// Класс клиента, который появляется на уровне и делает заказ
class Client {
public:
    string file;                // Файл текстуры персонажа
    Texture texture;
    Sprite sprite;
    float x, y;

    int frameCount;            // Кол-во кадров анимации
    int currentFrame;
    float frameDuration;
    float animationAccumulatedTime;

    enum ClientState { Growing, Waiting, StoryTelling, Leaving } state;

    float elapsedPatience;     // Сколько времени клиент ждал
    float patienceTime;        // Максимальное время ожидания
    bool isGone;

    vector<string> orders;         // Что заказал
    vector<string> servedOrders;   // Что получил

    // Флаги по ингредиентам (для будущей логики)
    bool requireSausage;
    bool requireCheese;
    bool requirePepper;
    bool requireChocolate;
    bool requireBanana;
    bool requireCake1;
    bool requireCake2;

    wstring name;              // Отображаемое имя клиента

    // Конструктор клиента с заказом и его характеристиками
    Client(const string& F, float X, float Y, float patienceSeconds,
        const vector<string>& clientOrders, bool sausage, bool cheese, bool pepper,
        bool chocolate, bool banana, bool cake1, bool cake2, const wstring& clientName)
        : file(F), orders(clientOrders), requireSausage(sausage), requireCheese(cheese),
        requirePepper(pepper), requireChocolate(chocolate), requireBanana(banana),
        requireCake1(cake1), requireCake2(cake2), x(322), y(59),
        currentFrame(0), animationAccumulatedTime(0.0f),
        state(Growing), patienceTime(patienceSeconds), isGone(false),
        name(clientName), elapsedPatience(0.0f)
    {
        servedOrders.clear();
        if (!texture.loadFromFile("Characters/" + F)) {
            cout << "Ошибка загрузки персонажа: Characters/" << F << endl;
            texture.create(294, 237);
            Uint8* pixels = new Uint8[294 * 237 * 4];
            for (int i = 0; i < 294 * 237 * 4; i += 4) {
                pixels[i] = 255; pixels[i + 1] = 0; pixels[i + 2] = 0; pixels[i + 3] = 255;
            }
            texture.update(pixels);
            delete[] pixels;
        }
        sprite.setTexture(texture);
        frameCount = 6;
        frameDuration = 0.05f;
        sprite.setTextureRect(IntRect(0, 0, 294, 237));
        sprite.setPosition(x, y);
    }

    // Добавление блюда в список отданных, если заказано
    bool serveDish(const string& dishType) {
        int orderedCount = count(orders.begin(), orders.end(), dishType);
        int servedCount = count(servedOrders.begin(), servedOrders.end(), dishType);
        if (servedCount < orderedCount) {
            servedOrders.push_back(dishType);
            return true;
        }
        return false;
    }

    // Проверка, все ли блюда выданы
    bool allOrdersServed() const {
        return servedOrders.size() == orders.size();
    }

    // Анимация появления/ухода клиента
    void updateAnimation(float dt) {
        if (state == Growing && currentFrame < frameCount - 1) {
            animationAccumulatedTime += dt;
            if (animationAccumulatedTime >= frameDuration) {
                animationAccumulatedTime -= frameDuration;
                currentFrame++;
                IntRect frameRect(currentFrame * 294, 0, 294, 237);
                sprite.setTextureRect(frameRect);
            }
        }
        else if (state == Growing && currentFrame == frameCount - 1) {
            state = Waiting;
        }
    }

    // Обновление таймера терпения и анимации
    void update(float dt, bool isGamePaused) {
        if (!isGamePaused) {
            updateAnimation(dt);
        }

        if (state == Waiting && !isGamePaused) {
            elapsedPatience += dt;
            if (elapsedPatience >= patienceTime) {
                state = Leaving;
                currentFrame = frameCount - 1;
            }
        }
        else if (state == Leaving && !isGamePaused) {
            animationAccumulatedTime += dt;
            if (animationAccumulatedTime >= frameDuration) {
                animationAccumulatedTime -= frameDuration;
                currentFrame--;
                if (currentFrame < 0) {
                    currentFrame = 0;
                    isGone = true;
                }
                IntRect frameRect(currentFrame * 294, 0, 294, 237);
                sprite.setTextureRect(frameRect);
            }
        }
    }

    // Переход в режим благодарности
    void completeOrder() {
        if (state == Waiting) {
            state = StoryTelling;
            currentFrame = frameCount - 1;
        }
    }

    // Если заказ не принят — клиент уходит
    void failOrder() {
        if (state == Waiting) {
            state = Leaving;
            currentFrame = frameCount - 1;
        }
    }

    bool isReadyForOrder() const {
        return state == Waiting;
    }
};

// Класс машины: готовит кофе или блюда
class Machine {
public:
    Texture textureIdle, texturePreparing, textureFinished; // Текстуры по состояниям
    Sprite sprite;
    float prepTime;            // Время приготовления
    enum MachineState { Idle, Preparing, Finished } state;
    string machineType;
    float preparationProgress;

    int animationFrames;
    int currentAnimationFrame;
    float animationDuration;
    float animationElapsedTime;
    IntRect animationRect;

    // Конструктор: принимает пути к изображениям, позицию и длительность приготовления
    Machine(string idleImg, string preparingImg, string finishedImg, Vector2f pos,
        float t, string type, int animFrames = 1)
        : prepTime(t), state(Idle), machineType(type), preparationProgress(0.0f),
        animationFrames(animFrames), currentAnimationFrame(0),
        animationDuration(0.1f), animationElapsedTime(0.0f)
    {
        // Загрузка всех фаз машины
        if (!textureIdle.loadFromFile(idleImg)) {
            cout << "Ошибка загрузки: " << idleImg << endl;
            textureIdle.create(277, 326);
        }
        if (!texturePreparing.loadFromFile(preparingImg)) {
            cout << "Ошибка загрузки: " << preparingImg << endl;
            texturePreparing.create(277, 326);
        }
        if (!textureFinished.loadFromFile(finishedImg)) {
            cout << "Ошибка загрузки: " << finishedImg << endl;
            textureFinished.create(277, 326);
        }

        sprite.setTexture(textureIdle);
        sprite.setPosition(pos);

        // Поддержка анимации, если несколько кадров
        if (animationFrames > 1) {
            float frameWidth = 277.8975f;
            animationRect = IntRect(0, 0, frameWidth, 326);
            sprite.setTextureRect(animationRect);
        }
    }

    // Запускает приготовление
    void start() {
        if (state == Idle) {
            state = Preparing;
            preparationProgress = 0.0f;
            sprite.setTexture(texturePreparing);
            currentAnimationFrame = 0;
            animationElapsedTime = 0.0f;
        }
    }

    // Обновление состояния машины
    void update(float dt, bool isGamePaused) {
        if (state == Preparing && !isGamePaused) {
            preparationProgress += dt / prepTime;

            // Анимация в процессе приготовления
            animationElapsedTime += dt;
            if (animationElapsedTime >= animationDuration) {
                animationElapsedTime = 0.0f;
                currentAnimationFrame = (currentAnimationFrame + 1) % animationFrames;
                animationRect.left = currentAnimationFrame * 277.8975f;
                sprite.setTextureRect(animationRect);
            }

            // Завершение
            if (preparationProgress >= 1.0f) {
                state = Finished;
                sprite.setTexture(textureFinished);
                sprite.setTextureRect(IntRect(0, 0, textureFinished.getSize().x, textureFinished.getSize().y));
                preparationProgress = 1.0f;
            }
        }
    }

    // Отрисовка машины и прогресс-бара
    void draw(RenderWindow& window) {
        window.draw(sprite);

        if (state == Preparing) {
            float barWidth = 100;
            float barHeight = 10;

            FloatRect bounds = sprite.getGlobalBounds();
            float centerX = bounds.left + bounds.width / 2.f;
            float topY = bounds.top - 15;

            RectangleShape progressBar(Vector2f(barWidth * preparationProgress, barHeight));
            progressBar.setFillColor(Color(70, 130, 180)); // стальной синий
            progressBar.setPosition(centerX - (barWidth / 2.f), topY);
            window.draw(progressBar);
        }
    }

    // Проверяет, был ли клик по машине
    bool isClicked(Vector2f mousePos) {
        return sprite.getGlobalBounds().contains(mousePos);
    }

    // Сброс машины в начальное состояние
    void reset() {
        state = Idle;
        preparationProgress = 0.0f;
        sprite.setTexture(textureIdle);
        if (animationFrames > 1) {
            sprite.setTextureRect(IntRect(0, 0, textureIdle.getSize().x, textureIdle.getSize().y));
        }
    }
};

// Кнопка включения/выключения звука — используется в игровом интерфейсе и меню паузы
class SoundButton {
public:
    Texture soundOnTexture;     // Иконка "звук включён"
    Texture soundOffTexture;    // Иконка "звук выключен"
    Sprite sprite;              // Спрайт кнопки

    float x, y;                 // Положение кнопки
    bool isHovered;            // Наведена ли мышь
    bool* soundState;          // Указатель на флаг включения звука
    bool prevSoundState;       // Предыдущее значение — для отслеживания изменений

    // Конструктор получает ссылку на глобальную переменную soundOn
    SoundButton(bool* soundOn) : soundState(soundOn), x(1180), y(20), isHovered(false), prevSoundState(*soundOn) {
        // Загрузка текстур
        if (!soundOffTexture.loadFromFile("Menu/sound_off.png"))
            cout << "Ошибка загрузки: Menu/sound_off.png" << endl;
        if (!soundOnTexture.loadFromFile("Menu/sound_on.png"))
            cout << "Ошибка загрузки: Menu/sound_on.png" << endl;

        // Установка начальной текстуры в зависимости от состояния
        if (*soundOn)
            sprite.setTexture(soundOnTexture);
        else
            sprite.setTexture(soundOffTexture);

        sprite.setPosition(x, y);
        
    }

    // Принудительно обновить текстуру (например, при возвращении на уровень)
    void forceUpdate() {
        updateTexture();
        prevSoundState = *soundState;
    }

    // Обновляет текстуру в зависимости от состояния
    void updateTexture() {
        if (*soundState)
            sprite.setTexture(soundOnTexture);
        else
            sprite.setTexture(soundOffTexture);
    }

    // Если состояние звука изменилось — обновить иконку
    void updateState() {
        if (*soundState != prevSoundState) {
            updateTexture();
            prevSoundState = *soundState;
        }
    }

    // Обновление флага наведения мыши
    void update(Vector2f mousePos) {
        FloatRect bounds = sprite.getGlobalBounds();
        isHovered = bounds.contains(mousePos);
    }

    // Проверка, был ли клик по кнопке
    bool contains(Vector2f point) {
        return sprite.getGlobalBounds().contains(point);
    }

    // Переключить звук: включить или выключить
    void toggle() {
        *soundState = !*soundState;
        updateTexture();
        prevSoundState = *soundState;
    }

    // Отрисовка кнопки
    void draw(RenderWindow& window) {
        window.draw(sprite);
    }
};

// Меню выбора уровня — список кнопок с открытыми уровнями и кнопка "назад"
class LevelSelectMenu {
public:
    Texture bgTexture;      // Фон
    Sprite bgSprite;
    SoundBuffer clickBuffer;
    Sound clickSound;

    struct LevelButton {
        Texture normalTex;  // Текстура обычного состояния
        Texture hoverTex;   // Текстура при наведении
        Sprite sprite;
        float x, y;         // Положение кнопки
        float width, height;
        float hoverY;
        int level;          // Какой уровень запускает
        bool isHovered;
    };

    vector<LevelButton> buttons;
    SoundButton soundButton;     // Кнопка звука в углу
    SaveManager* saveManager;    // Доступ к информации о разблокированных уровнях

    // Создание меню — принимает указатели на менеджер сохранения и флаг звука
    LevelSelectMenu(SaveManager* saveManager, bool* soundState)
        : saveManager(saveManager), soundButton(soundState)
    {
        // Загрузка фона
        if (!bgTexture.loadFromFile("Menu/level_select_bg.png")) {
            cout << "Ошибка загрузки фона меню уровней" << endl;
            bgTexture.create(1280, 720);
        }
        bgSprite.setTexture(bgTexture);

        // Звук клика
        if (!clickBuffer.loadFromFile("audio/button_click.wav")) {
            cout << "Ошибка загрузки звука кнопки" << endl;
        }
        clickSound.setBuffer(clickBuffer);

        // Расчёт координат сетки
        float startX = 340;
        float startY = 230;
        float spacingX = 250;
        float spacingY = 200;
        int maxLevelsInRow = 3;

        // Создаём кнопки уровней, если они открыты
        for (int level = 1; level <= 6; level++) {
            if (level <= saveManager->getMaxUnlockedLevel()) {
                float x = startX + ((level - 1) % maxLevelsInRow) * spacingX;
                float y = startY + ((level - 1) / maxLevelsInRow) * spacingY;

                createLevelButton("Menu/level" + to_string(level) + ".png",
                    "Menu/level" + to_string(level) + "_hover.png",
                    x, y, y + 5, level);
            }
        }

        // Кнопка "назад"
        createLevelButton("Menu/back.png", "Menu/Back_hover.png", 590, 600, 605, 0);

        update(0, 0); // Инициализация состояния наведения
    }

    // Создаёт одну кнопку уровня или "назад"
    void createLevelButton(const string& normalFile, const string& hoverFile,
        float x, float y, float hoverY, int level) {
        LevelButton btn;
        btn.level = level;
        btn.x = x;
        btn.y = y;
        btn.hoverY = hoverY;
        btn.isHovered = false;

        if (!btn.normalTex.loadFromFile(normalFile)) {
            cout << "Ошибка загрузки: " << normalFile << endl;
            btn.normalTex.create(150, 50);
        }
        btn.sprite.setTexture(btn.normalTex);

        if (!btn.hoverTex.loadFromFile(hoverFile)) {
            cout << "Ошибка загрузки hover: " << hoverFile << endl;
            btn.hoverTex = btn.normalTex;
        }

        btn.sprite.setPosition(x, y);
        btn.width = static_cast<float>(btn.normalTex.getSize().x);
        btn.height = static_cast<float>(btn.normalTex.getSize().y);
        buttons.push_back(btn);
    }

    // Наведение мыши — смена спрайтов
    void update(float mouseX, float mouseY) {
        soundButton.update(Vector2f(mouseX, mouseY));

        for (auto& btn : buttons) {
            if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
                mouseY >= btn.y && mouseY <= btn.y + btn.height) {
                btn.isHovered = true;
                btn.sprite.setTexture(btn.hoverTex);
                btn.sprite.setPosition(btn.x, btn.hoverY);
            }
            else {
                btn.isHovered = false;
                btn.sprite.setTexture(btn.normalTex);
                btn.sprite.setPosition(btn.x, btn.y);
            }
        }
    }

    // Обработка клика мышкой
    int handleClick(float mouseX, float mouseY) {
        Vector2f mousePos(mouseX, mouseY);

        if (soundButton.contains(mousePos)) {
            soundButton.toggle();
            return -1;
        }

        for (auto& btn : buttons) {
            if (btn.isHovered) {
                if (soundButton.soundState && *soundButton.soundState) {
                    clickSound.play();
                }
                return btn.level; // Вернёт 0 — назад, либо номер уровня
            }
        }

        return -1;
    }

    // Отрисовка экрана меню уровней
    void draw(RenderWindow& window) {
        window.draw(bgSprite);
        for (auto& btn : buttons) {
            window.draw(btn.sprite);
        }
        soundButton.draw(window);
    }
};

// Класс перетаскиваемого предмета — ингредиенты, напитки и т.п.
class DraggableItem {
public:
    static map<string, Texture> preloadedTextures; // Предзагруженные текстуры всех видов предметов

    Texture* texturePtr;        // Указатель на текстуру
    Sprite sprite;              // Визуальный объект
    string type;                // Тип предмета ("coffee", "sausage" и др.)
    Vector2f originalPosition;  // Исходная точка
    Vector2f dragOffset;        // Смещение курсора относительно спрайта
    bool isDragging;            // В процессе ли перетаскивания
    bool active;                // Активен ли (для удаления или скрытия)
    bool isStatic;              // Является ли объектом одного использования

    // Предзагружает все нужные текстуры (единожды для всех объектов)
    static void preloadAllTextures() {
        vector<string> items = {
            "sausage", "pepper", "cheese", "banana",
            "chocolate", "coffee", "cake1", "cake2"
        };

        for (const auto& item : items) {
            string filename = item;

            // Именование отдельного случая
            if (item == "coffee") {
                filename = "coffee_cup";
            }

            Texture texture;
            if (!texture.loadFromFile("Ingredients/" + item + ".png")) {
                cout << "Ошибка загрузки: Ingredients/" << item << ".png" << endl;

                // Заглушка на случай ошибки загрузки
                texture.create(50, 50);
                Uint8* pixels = new Uint8[50 * 50 * 4];
                for (int i = 0; i < 50 * 50 * 4; i += 4) {
                    pixels[i] = 255; pixels[i + 1] = 0;
                    pixels[i + 2] = 0;   pixels[i + 3] = 255;
                }
                texture.update(pixels);
                delete[] pixels;
            }

            preloadedTextures[item] = texture;
        }

        cout << "Предзагружено " << preloadedTextures.size() << " текстур ингредиентов" << endl;
    }

    // Конструктор: задаётся тип, позиция и флаг статичности
    DraggableItem(const string& itemType, Vector2f pos, bool isStaticItem = false)
        : type(itemType), isDragging(false), active(true), isStatic(isStaticItem)
    {
        auto it = preloadedTextures.find(itemType);
        texturePtr = (it != preloadedTextures.end()) ? &it->second : &preloadedTextures.begin()->second;

        sprite.setTexture(*texturePtr);
        sprite.setPosition(pos);
        originalPosition = pos;
    }

    // Начинает перетаскивание (если клик попал в границы)
    void startDrag(Vector2f mousePos) {
        if (sprite.getGlobalBounds().contains(mousePos)) {
            isDragging = true;
            dragOffset = sprite.getPosition() - mousePos;
        }
    }

    // Перемещает предмет вслед за курсором
    void drag(Vector2f mousePos) {
        if (isDragging) {
            sprite.setPosition(mousePos + dragOffset);
        }
    }

    // Отпускает предмет (прекращает перетаскивание)
    void stopDrag() {
        isDragging = false;
    }

    // Возвращает предмет на исходную позицию
    void resetPosition() {
        sprite.setPosition(originalPosition);
    }

    // Деактивирует объект (например, после использования)
    void setInactive() {
        active = false;
    }

    // Повторно активирует и возвращает на место
    void setActive() {
        active = true;
        resetPosition();
    }

    // Проверяет, находится ли курсор внутри объекта
    bool contains(Vector2f point) {
        return active && sprite.getGlobalBounds().contains(point);
    }
};

// Инициализация статического поля текстур
map<string, Texture> DraggableItem::preloadedTextures;

// Класс блюда-круассана — можно добавить шоколад и/или банан
class CroissantDish {
public:
    enum Ingredient { NONE, CHOCOLATE, BANANA, BOTH }; // Состояния комбинации
    Ingredient currentState;
    Sprite sprite;
    Vector2f position;

    // Текстуры разных вариантов круассана
    Texture emptyTex;
    Texture chocolateTex;
    Texture bananaTex;
    Texture bothTex;

    bool isReady;       // Готов ли к подаче
    bool isDragging;    // Перетаскивается ли

    CroissantDish(Vector2f pos)
        : position(pos), currentState(NONE), isReady(false), isDragging(false)
    {
        if (!emptyTex.loadFromFile("Dishes/empty.png"))
            emptyTex.create(100, 100);
        if (!chocolateTex.loadFromFile("Dishes/with_chocolate.png"))
            chocolateTex.create(100, 100);
        if (!bananaTex.loadFromFile("Dishes/with_banana.png"))
            bananaTex.create(100, 100);
        if (!bothTex.loadFromFile("Dishes/with_both.png"))
            bothTex.create(100, 100);

        sprite.setTexture(emptyTex);
        sprite.setPosition(position);
    }

    // Добавляет ингредиент к круассану и обновляет внешний вид
    void addIngredient(const string& ingredient) {
        if (ingredient == "chocolate") {
            if (currentState == NONE) currentState = CHOCOLATE;
            else if (currentState == BANANA) currentState = BOTH;
        }
        else if (ingredient == "banana") {
            if (currentState == NONE) currentState = BANANA;
            else if (currentState == CHOCOLATE) currentState = BOTH;
        }
        updateTexture();
    }

    // Сброс позиции и состояния
    void resetPosition() {
        sprite.setPosition(position);
    }

    void reset() {
        currentState = NONE;
        isReady = false;
        sprite.setTexture(emptyTex);
        sprite.setPosition(position);
    }

    // Обновление текстуры в зависимости от состояния
    void updateTexture() {
        switch (currentState) {
        case NONE: sprite.setTexture(emptyTex); break;
        case CHOCOLATE: sprite.setTexture(chocolateTex); break;
        case BANANA: sprite.setTexture(bananaTex); break;
        case BOTH:
            sprite.setTexture(bothTex);
            isReady = true;
            break;
        }
    }

    // Перетаскивание
    void startDrag(Vector2f mousePos) {
        if (sprite.getGlobalBounds().contains(mousePos)) {
            isDragging = true;
        }
    }

    void drag(Vector2f mousePos) {
        if (isDragging) {
            sprite.setPosition(mousePos);
        }
    }

    void stopDrag() {
        isDragging = false;
    }

    bool contains(Vector2f point) {
        return sprite.getGlobalBounds().contains(point);
    }
};

// Класс блюда-сэндвича — комбинация из сосиски, сыра и перца
class SandwichDish {
public:
    bool hasSausage, hasCheese, hasPepper;
    Sprite sprite;
    Vector2f position;
    Texture textures[8];     // Все варианты сэндвича (2^3 = 8)
    bool isReady;
    bool isDragging;
    Vector2f dragOffset;

    SandwichDish(Vector2f pos)
        : position(pos), hasSausage(false), hasCheese(false), hasPepper(false),
        isReady(false), isDragging(false)
    {
        for (int i = 0; i < 8; i++) {
            string filename = "Dishes/sandwich_" + to_string(i) + ".png";
            if (!textures[i].loadFromFile(filename)) {
                textures[i].create(100, 100); // Заглушка
            }
        }

        sprite.setTexture(textures[0]);
        sprite.setPosition(position);
    }

    // Добавляет ингредиент
    void addIngredient(const string& ingredient) {
        if (ingredient == "sausage") hasSausage = true;
        else if (ingredient == "cheese") hasCheese = true;
        else if (ingredient == "pepper") hasPepper = true;

        updateTexture();
    }

    // Обновляет текстуру по текущему составу
    void updateTexture() {
        int state = 0;
        if (hasSausage) state |= 1;
        if (hasCheese)  state |= 2;
        if (hasPepper)  state |= 4;

        if (state >= 0 && state < 8)
            sprite.setTexture(textures[state]);

        isReady = true;
    }

    void resetPosition() {
        sprite.setPosition(position);
    }

    void reset() {
        hasSausage = hasCheese = hasPepper = false;
        isReady = false;
        sprite.setTexture(textures[0]);
        sprite.setPosition(position);
    }

    // Управление перетаскиванием
    void startDrag(Vector2f mousePos) {
        if (sprite.getGlobalBounds().contains(mousePos)) {
            isDragging = true;
            dragOffset = sprite.getPosition() - mousePos;
        }
    }

    void drag(Vector2f mousePos) {
        if (isDragging) {
            sprite.setPosition(mousePos + dragOffset);
        }
    }

    void stopDrag() {
        isDragging = false;
    }

    bool contains(Vector2f point) {
        return sprite.getGlobalBounds().contains(point);
    }
};

// Экран результатов после завершения уровня
class LevelResultsScreen {
public:
    Texture bgTexture;        // Фон
    Sprite bgSprite;

    Texture buttonNormal;     // Кнопка "Продолжить"
    Texture buttonHover;      // Кнопка при наведении
    Sprite buttonSprite;

    FloatRect buttonBounds;   // Область кнопки
    bool buttonHovered;

    Text resultsText;         // Основной текст с итогами
    Font font;

    LevelResultsScreen() : buttonHovered(false) {
        // Загрузка фона
        if (!bgTexture.loadFromFile("Menu/level_final_screen.png")) {
            bgTexture.create(1280, 720);
        }
        bgSprite.setTexture(bgTexture);

        // Загрузка текстур кнопки
        if (!buttonNormal.loadFromFile("Menu/final_button_normal.png")) {
            buttonNormal.create(266, 150);
        }
        if (!buttonHover.loadFromFile("Menu/final_button_hover.png")) {
            buttonHover = buttonNormal;
        }

        buttonSprite.setTexture(buttonNormal);
        buttonSprite.setPosition(507, 531);
        buttonBounds = buttonSprite.getGlobalBounds();

        // Загрузка шрифта
        if (!font.loadFromFile("Montserrat-SemiBold.ttf")) {
            cout << "Ошибка загрузки шрифта в экране результатов" << endl;
        }

        // Настройка текста
        resultsText.setFont(font);
        resultsText.setCharacterSize(36);
        resultsText.setFillColor(Color(164, 106, 72));
        resultsText.setOutlineColor(Color(62, 29, 6));
        resultsText.setOutlineThickness(1);
        resultsText.setPosition(300, 300);
    }

    // Запись финального результата
    void setResults(int level, int served, int total, float totalTime) {
        wstringstream ss;
        ss << L"Уровень " << level << L" завершен!\n";
        ss << L"Клиентов обслужено: " << served << L" из " << total << L"\n";
        ss << L"Общее время: " << fixed << setprecision(1) << totalTime << L" сек\n";
        resultsText.setString(ss.str());
    }

    // Обновляет состояние наведения мыши
    void update(Vector2f mousePos) {
        buttonHovered = buttonBounds.contains(mousePos);
        buttonSprite.setTexture(buttonHovered ? buttonHover : buttonNormal);
    }

    // Отрисовывает экран и элементы
    void draw(RenderWindow& window) {
        window.draw(bgSprite);
        window.draw(resultsText);
        window.draw(buttonSprite);
    }
};

// Главный класс игры — управляет окнами, состоянием и логикой
class Game {
private:
    // Меню справки
    RectangleShape menuBar;
    Text helpMenuText;
    bool helpMenuHovered;
    FloatRect helpMenuBounds;
    Font menuFont;

    RenderWindow window;     // Главное окно игры
    Font font;               // Общий шрифт

    LevelSelectMenu* levelSelectMenu;  // Меню выбора уровня
    Menu mainMenu;                     // Главное меню
    LoadingScreen loadingScreen;       // Заставка загрузки

    SoundButton gameSoundButton;       // Кнопка звука в игре
    SoundBuffer clickBuffer;           // Буфер звука клика
    Sound clickSound;                  // Звук клика

    // Состояния игры
    enum GameState { LOADING, MAIN_MENU, LEVEL_SELECT, GAME_PLAY, FINAL_SCREEN, LEVEL_RESULTS } state;

    Client* currentClient;      // Текущий клиент на экране
    int clientsServed;          // Сколько обслужено за уровень
    int successfulServed;       // Сколько успешно

    // Истории персонажей по уровням
    map<int, map<string, vector<wstring>>> levelStories;

    Machine* coffeeMachine;     // Кофейная машина
    Clock deltaClock;           // Таймер обновлений
    vector<string> currentOrders; // Текущий заказ

    Music backgroundMusic;      // Фоновая музыка
    bool soundOn;               // Флаг звука

    SaveManager saveManager;    // Сохраняет уровень и прогресс
    int currentLevelNumber;     // Активный уровень

    // Графика уровня
    Texture levelBgTexture;
    Sprite levelBgSprite;

    Texture counterTexture;
    Sprite counterSprite;

    OrderWindow orderWindow;    // Окно заказа
    DialogCloud dialogCloud;    // Облако диалога

    enum OrderState { WAITING, PREPARING, READY_TO_SERVE } orderState;
    bool orderCorrect;

    float coffeePreparationTime; // Время готовки кофе

    // Блюда
    CroissantDish* croissantDish;
    SandwichDish* sandwichDish;

    // Для перетаскивания
    CroissantDish* currentDragCroissant;
    SandwichDish* currentDragSandwich;
    DraggableItem* currentDragItem;

    vector<string> availableCharacters; // Очередь персонажей
    Clock clientSpawnTimer;
    bool spawningClient;

    // Области для объектов
    FloatRect croissantArea;
    FloatRect sandwichArea;
    FloatRect sausageArea;
    FloatRect pepperArea;
    FloatRect cheeseArea;
    FloatRect bananaArea;
    FloatRect chocolateArea;
    FloatRect coffeeArea;
    FloatRect cake1Area;
    FloatRect cake2Area;
    FloatRect trashArea;

    vector<DraggableItem> activeIngredients; // Все ингредиенты на сцене

    // Финальный экран
    Texture finalScreenTexture;
    Sprite finalScreenSprite;
    Texture finalButtonNormal;
    Texture finalButtonHover;
    Sprite finalButtonSprite;
    bool finalButtonHovered;
    FloatRect finalButtonBounds;

    LevelResultsScreen levelResultsScreen;
    float levelElapsedTime;
    float currentClientSpawnTime;
    vector<float> clientTimes;

    // Пауза
    bool gamePaused;
    Texture pauseButtonNormalTex;
    Texture pauseButtonHoverTex;
    Sprite pauseButtonSprite;
    bool pauseButtonHovered;

    Texture pauseOverlayTex;
    Sprite pauseOverlaySprite;

    Texture playButtonNormalTex;
    Texture playButtonHoverTex;
    Sprite playButtonSprite;
    bool playButtonHovered;
    FloatRect playButtonBounds;

    Texture mainMenuButtonNormalTex;
    Texture mainMenuButtonHoverTex;
    Sprite mainMenuButtonSprite;
    bool mainMenuButtonHovered;
    FloatRect mainMenuButtonBounds;

    bool isFinalLevelCompleted;  // Флаг завершения финального уровня

    
    // Сброс текущего уровня: очищает ингредиенты, сбрасывает машины и диалог
    void resetLevel() {
        activeIngredients.clear();       // Удаляем все активные (перетаскиваемые) ингредиенты
        availableCharacters.clear();     // Очищаем очередь доступных клиентов
        isFinalLevelCompleted = false;  // Сброс флага при новом уровне

        // Удаляем круассан, если он есть
        if (croissantDish) {
            delete croissantDish;
            croissantDish = nullptr;
        }

        // Удаляем сэндвич, если он есть
        if (sandwichDish) {
            delete sandwichDish;
            sandwichDish = nullptr;
        }

        coffeeMachine->reset();          // Сбрасываем кофемашину в начальное состояние

        // Убираем указатели на объекты, которые могли перетаскиваться
        currentDragItem = nullptr;
        currentDragCroissant = nullptr;
        currentDragSandwich = nullptr;

        orderState = WAITING;            // Состояние заказа — ждём нового клиента
        orderWindow.visible = false;     // Скрываем окно заказа

        clientsServed = 0;               // Счётчики для текущего уровня
        successfulServed = 0;
        clientTimes.clear();             // Обнуляем список времен обслуживания

        gameSoundButton.forceUpdate();   // Обновляем кнопку звука (на случай возврата)

        // Очищаем облако диалога
        dialogCloud.visible = false;
        dialogCloud.completed = true;
        dialogCloud.currentPhrase = 0;
        dialogCloud.phrases.clear();
        dialogCloud.dialogText.setString(L"");
    }

    void loadFinalScreen() {
        if (!finalScreenTexture.loadFromFile("Menu/final_screen.png")) {
            finalScreenTexture.create(1280, 720);
        }
        finalScreenSprite.setTexture(finalScreenTexture);
    }


public:
    Game()

        // Создаём главное окно размером 1280x720 с заголовком и ограниченными стилями
        : window(VideoMode(1280, 720), L"Кафе Мечта", Style::Titlebar | Style::Close),

        // Инициализация начального состояния игры
        state(LOADING),
        soundOn(true),
        saveManager(),

        // Передаём ссылки на менеджер сохранения и флаг звука в главное меню
        mainMenu(&saveManager, &soundOn),

        // Пока меню уровней не создано
        levelSelectMenu(nullptr),

        // Кнопка звука в игровом интерфейсе
        gameSoundButton(&soundOn),

        // Инициализация указателей и счётчиков
        currentClient(nullptr),
        clientsServed(0),
        successfulServed(0),

        // Загружаем текущий уровень из сохранения
        currentLevelNumber(saveManager.getCurrentLevel()),

        // Состояние заказа и кофе-машины
        orderState(WAITING),
        orderCorrect(false),
        coffeePreparationTime(5.0f),

        // Указатели на блюда
        croissantDish(nullptr),
        sandwichDish(nullptr),
        currentDragCroissant(nullptr),
        currentDragSandwich(nullptr),
        currentDragItem(nullptr),

        // Генерация клиентов и координаты игровых зон
        spawningClient(false),
        croissantArea(1092.5f, 590, 178.5f, 91),
        sandwichArea(950, 619, 155, 74),
        sausageArea(839, 415, 62, 103),
        pepperArea(908, 408, 72.43, 88.84),
        cheeseArea(6.68, 644.71, 119, 58),
        bananaArea(768, 303, 140, 108),
        chocolateArea(821, 634, 37, 84),
        coffeeArea(113.37f, 339.77f, 95, 77.65f),
        cake1Area(1034.75f, 219, 159.19f, 114.35f),
        cake2Area(1091, 356, 159.19f, 107.72f),
        trashArea(275, 608, 100, 100),

        // Финальный экран и таймер уровня
        finalButtonHovered(false),
        levelElapsedTime(0.0f),
        currentClientSpawnTime(0.0f),

        // Состояние паузы и кнопок в паузе
        gamePaused(false),
        pauseButtonHovered(false),
        playButtonHovered(false),
        mainMenuButtonHovered(false),
        isFinalLevelCompleted(false)
    {
        // Загружаем иконку приложения
        Image icon;
        if (icon.loadFromFile("Menu/app_icon.png")) {
            window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
        }
        else {
            cout << "Не удалось загрузить иконку окна" << endl;
        }

        // Синхронизируем иконку звука в главном меню
        mainMenu.updateSoundButton();

        // Предзагрузка всех текстур для заказов и ингредиентов
        OrderWindow::preloadAllTextures();
        DraggableItem::preloadAllTextures();

        // Ограничение FPS до 60 кадров в секунду
        window.setFramerateLimit(60);

        // Загрузка звука нажатия на кнопки
        if (!clickBuffer.loadFromFile("audio/button_click.wav")) {
            cout << "Ошибка загрузки звука кнопки" << endl;
        }
        clickSound.setBuffer(clickBuffer);

        // Загрузка основного шрифта
        if (!font.loadFromFile("Montserrat-SemiBold.ttf")) {
            cout << "Ошибка загрузки шрифта" << endl;
        }

        if (!menuFont.loadFromFile("Montserrat-SemiBold.ttf")) {
    cout << "Ошибка загрузки шрифта меню" << endl;
    menuFont = font;
}

        // Инициализация панели меню
        menuBar.setSize(Vector2f(1280, 30));
        menuBar.setFillColor(Color(60, 60, 80));  // Тёмно-синий цвет
        menuBar.setPosition(0, 0);

        // Инициализация пункта меню "Справка"
        helpMenuText.setFont(menuFont);
        helpMenuText.setString(L"Справка");
        helpMenuText.setCharacterSize(20);
        helpMenuText.setFillColor(Color::White);
        helpMenuText.setPosition(20, 5);
        helpMenuBounds = helpMenuText.getGlobalBounds();
        helpMenuHovered = false;

        // Загрузка фона уровня
        if (!levelBgTexture.loadFromFile("Levels/level_bg.png")) {
            levelBgTexture.create(1280, 720);
        }
        levelBgSprite.setTexture(levelBgTexture);

        // Загрузка текстуры прилавка
        if (!counterTexture.loadFromFile("Levels/counter.png")) {
            counterTexture.create(800, 200);
        }
        counterSprite.setTexture(counterTexture);
        counterSprite.setPosition(0, 0);

        // Загрузка и запуск фоновой музыки
        if (!backgroundMusic.openFromFile("audio/background_music.wav")) {
            cout << "Ошибка загрузки фоновой музыки" << endl;
        }
        else {
            backgroundMusic.setLoop(true);
            backgroundMusic.play();
            backgroundMusic.setVolume(50);
        }

        // Инициализация кофе-машины с 4 кадрами анимации
        coffeeMachine = new Machine(
            "coffee_idle.png",
            "coffee_preparing.png",
            "coffee_finished.png",
            Vector2f(39.81f, 138.71f),
            5.0f,
            "coffee",
            4
        );

        // Загрузка и установка кнопок паузы, продолжения и выхода в меню паузы
        if (!pauseButtonNormalTex.loadFromFile("Menu/pause_button.png"))
            pauseButtonNormalTex.create(50, 50);
        if (!pauseButtonHoverTex.loadFromFile("Menu/pause_button_hover.png"))
            pauseButtonHoverTex = pauseButtonNormalTex;

        pauseButtonSprite.setTexture(pauseButtonNormalTex);
        pauseButtonSprite.setPosition(1103, 44);

        // Полупрозрачная затемнённая подложка для меню паузы
        if (!pauseOverlayTex.loadFromFile("Menu/pause_overlay.png")) {
            pauseOverlayTex.create(1280, 720);
            Uint8* pixels = new Uint8[1280 * 720 * 4];
            for (int i = 0; i < 1280 * 720 * 4; i += 4) {
                pixels[i] = 0; pixels[i + 1] = 0; pixels[i + 2] = 0; pixels[i + 3] = 150;
            }
            pauseOverlayTex.update(pixels);
            delete[] pixels;
        }
        pauseOverlaySprite.setTexture(pauseOverlayTex);

        // Кнопка продолжения из паузы
        if (!playButtonNormalTex.loadFromFile("Menu/play_button_normal.png"))
            playButtonNormalTex.create(100, 100);
        if (!playButtonHoverTex.loadFromFile("Menu/play_button_hover.png"))
            playButtonHoverTex = playButtonNormalTex;

        playButtonSprite.setTexture(playButtonNormalTex);
        FloatRect playBounds = playButtonSprite.getLocalBounds();
        playButtonSprite.setOrigin(playBounds.width / 2, playBounds.height / 2);
        playButtonSprite.setPosition(1280 / 2, 720 / 2 - 50);
        playButtonBounds = playButtonSprite.getGlobalBounds();

        // Кнопка возврата в главное меню из паузы
        if (!mainMenuButtonNormalTex.loadFromFile("Menu/main_menu_button_normal.png"))
            mainMenuButtonNormalTex.create(400, 50);
        if (!mainMenuButtonHoverTex.loadFromFile("Menu/main_menu_button_hover.png"))
            mainMenuButtonHoverTex = mainMenuButtonNormalTex;

        mainMenuButtonSprite.setTexture(mainMenuButtonNormalTex);
        FloatRect menuBounds = mainMenuButtonSprite.getLocalBounds();
        mainMenuButtonSprite.setOrigin(menuBounds.width / 2, menuBounds.height / 2);
        mainMenuButtonSprite.setPosition(1280 / 2, 720 / 2 + 90);
        mainMenuButtonBounds = mainMenuButtonSprite.getGlobalBounds();

        // Загрузка всех диалогов персонажей по уровням
        loadStories();

        // Загрузка текстур финального экрана
        loadFinalScreen();
    }

    // Освобождаем ресурсы при завершении игры
    ~Game() {
        if (levelSelectMenu) delete levelSelectMenu;     // Удаляем меню выбора уровня
        if (currentClient) delete currentClient;         // Освобождаем текущего клиента
        delete coffeeMachine;                            // Удаляем кофе-машину
        if (croissantDish) delete croissantDish;         // Удаляем текущий круассан
        if (sandwichDish) delete sandwichDish;           // Удаляем текущий сэндвич
    }

    void loadStories() {
        levelStories.clear();

        levelStories[1]["character1"] = {
            L"12 издательств сказали 'нет'... Но Гарри заслуживает жизнь.",
            L"Буду писать здесь каждый день. Хотя бы строчку."
        };
        levelStories[1]["character2"] = {
            L"Украли моего Освальда... Но я придумал лучше!",
            L"Микки... да, он заговорит! Все назовут безумцем? Пусть!"
        };
        levelStories[1]["character3"] = {
            L"Falcon 1 взорвался. Третий раз. Денег почти нет.",
            L"Четвертый запуск будет. Марс не покорится без жертв."
        };

        levelStories[2]["character1"] = {
            L"Дочка плачет... а я пишу. Иногда кажется, это эгоизм.",
            L"Но если сдамся сейчас, кто расскажет миру о Гарри?"
        };
        levelStories[2]["character2"] = {
            L"Звук в 'Пароходике Вилли'? Кредит под залог дома? Безумие!",
            L"Но без риска нет магии. Микки ДОЛЖЕН заговорить!"
        };
        levelStories[2]["character3"] = {
            L"Все вложил в четвертый запуск. Банкротство или Марс.",
            L"Цель ясна. Риск оправдан. Точка."
        };

        levelStories[3]["character1"] = {
            L"'Блумсбери' сказали ДА! Крошечный тираж... но ДА!",
            L"Видишь? 'Нет' — это просто 'напиши еще страницу'."
        };
        levelStories[3]["character2"] = {
            L"Очереди! Овации! Микки и звук — СЕНСАЦИЯ!",
            L"Теперь 'Белоснежка'. Полный метр! Все говорят 'глупость'? Посмотрим!"
        };
        levelStories[3]["character3"] = {
            L"Орбита! Falcon 1 на орбите!",
            L"Это только начало. Следующая цель — МКС. Dragon готовится."
        };

        levelStories[4]["character1"] = {
            L"Книга 2, книга 3... Миллионы читают! Это сон?",
            L"Нет, это тысячи часов в кафе с пустым кошельком."
        };
        levelStories[4]["character2"] = {
            L"Аниматоры не спят неделями. Я — тем более. Каждый кадр — шедевр.",
            L"Кредиты огромные... но 'Белоснежка' изменит все кино!"
        };
        levelStories[4]["character3"] = {
            L"Tesla едва на плаву, SpaceX гложут скептики... Шум.",
            L"Цель неизменна: электромобили для всех, люди на Марсе. Работаем."
        };

        levelStories[5]["character1"] = {
            L"Премьера фильма! Миллионы смотрят Гарри! Из пепла...",
            L"...поднялась не просто книга — целый мир веры в чудо."
        };
        levelStories[5]["character2"] = {
            L"Семь 'Оскаров'! Один большой... и семь маленьких!",
            L"Белоснежка ожила! 'Диснеевская глупость' построила замки!"
        };
        levelStories[5]["character3"] = {
            L"Dragon пристыковался! Первая частная компания у МКС! История!",
            L"Видишь эту чашку? Капля кофе — неудача. Целая чашка — полет. Starship — следующий!"
        };

        levelStories[6]["character1"] = {
            L"Помню этот дешевый кофе... Он горел, но гнал вперед.",
            L"Секрет? Пиши. Даже когда все кричат 'Сдавайся!'."
        };
        levelStories[6]["character2"] = {
            L"Замок мечты открыт. Миллионы детей смеются!",
            L"Бери карандаш, верь в глупость... и работай, пока мечта не станет замком!"
        };
        levelStories[6]["character3"] = {
            L"Falcon Heavy, Starship... Орбита — стартовая площадка. Цель — Марс.",
            L"Мечта + Цель + Работа × Риск = Реальность. Формула проста."
        };
    }

    void spawnClient() {
        // Если предыдущий клиент ещё существует — удаляем его
        if (currentClient) delete currentClient;

        // Если список доступных персонажей пуст — создаём новый набор и перемешиваем его
        if (availableCharacters.empty()) {
            availableCharacters = { "character1.png", "character2.png", "character3.png" };
            random_shuffle(availableCharacters.begin(), availableCharacters.end()); // случайный порядок появления
        }

        // Скрываем облако с диалогом, чтобы не перекрывало нового клиента
        dialogCloud.visible = false;
        dialogCloud.completed = true;

        // Получаем имя файла текстуры нового клиента (последнего в перемешанном списке)
        string characterFile = availableCharacters.back();
        availableCharacters.pop_back(); // удаляем его из списка, чтобы не повторился в текущем цикле

        // Возможные блюда, которые может заказать клиент
        vector<string> possibleOrders = {
            "coffee",
            "croissant_plain", "croissant_chocolate", "croissant_banana", "croissant_both",
            "sandwich_plain", "sandwich_sausage", "sandwich_cheese", "sandwich_pepper",
            "sandwich_sausage_cheese", "sandwich_sausage_pepper", "sandwich_cheese_pepper", "sandwich_all",
            "cake1", "cake2"
        };

        // Определяем количество заказов в зависимости от номера уровня
        int orderCount;
        if (currentLevelNumber <= 2) {
            orderCount = 1; // на ранних уровнях один заказ
        }
        else if (currentLevelNumber <= 4) {
            orderCount = 2 + rand() % 2; // 2 или 3 заказа
        }
        else {
            orderCount = 3 + rand() % 2; // 3 или 4 на поздних уровнях
        }

        // Генерируем случайные заказы для клиента
        vector<string> orders;
        for (int i = 0; i < orderCount; i++) {
            string order = possibleOrders[rand() % possibleOrders.size()];
            orders.push_back(order);
        }

        // Задаём терпение клиента в секундах (с каждым уровнем уменьшается)
        float clientPatience = max(10.0f, 40.0f - ((currentLevelNumber - 1) / 2) * 5.0f);

        // Определяем имя клиента по файлу
        wstring clientName;
        if (characterFile == "character1.png") {
            clientName = L"Джоан Роулинг";
        }
        else if (characterFile == "character2.png") {
            clientName = L"Уолт Дисней";
        }
        else if (characterFile == "character3.png") {
            clientName = L"Илон Маск";
        }

        // Создаём нового клиента с нужными параметрами
        currentClient = new Client(
            characterFile,
            500, 200,
            clientPatience,
            orders,
            false, false, false, false, false, false, false, // флаги ингредиентов (пока не используются)
            clientName
        );

        currentOrders = orders; // сохраняем текущие заказы

        // Устанавливаем визуальное положение окна заказа и облака диалога
        orderWindow.setOrders(orders);
        orderWindow.setPosition(Vector2f(515, 15));
        dialogCloud.setPosition(Vector2f(550, 50));
        orderWindow.visible = false;   // сначала окно скрыто
        orderState = WAITING;          // состояние клиента — ожидает заказ

        currentClientSpawnTime = levelElapsedTime; // запоминаем время появления клиента
    }

    void prepareCoffee() {
        coffeeMachine->start();
        orderState = PREPARING;
    }

    // Функция обработки успешной подачи заказа клиенту
    void serveOrder() {
        // Проверка: все ли блюда из заказа уже выданы
        if (currentClient && currentClient->allOrdersServed()) {
            // Скрываем окно заказа
            orderWindow.visible = false;

            // Получаем имя клиента из файла (удаляем расширение)
            string characterFile = currentClient->file;
            size_t dotPos = characterFile.find('.');
            string characterName = (dotPos != string::npos)
                ? characterFile.substr(0, dotPos)
                : characterFile;

            // Пытаемся найти истории для текущего уровня и персонажа
            auto levelIt = levelStories.find(currentLevelNumber);
            if (levelIt != levelStories.end()) {
                auto charIt = levelIt->second.find(characterName);
                if (charIt != levelIt->second.end()) {
                    // Запускаем облако с диалогом персонажа
                    dialogCloud.start(charIt->second);
                }
            }

            // Переводим клиента в состояние благодарности/диалога
            currentClient->completeOrder();

            // Увеличиваем счётчик успешно обслуженных клиентов
            successfulServed++;
        }
    }

    Clock loadingClock;

    // Главный игровой цикл — запускается один раз, управляет всей логикой и отображением
    void run() {
        while (window.isOpen()) {
            // Считаем, сколько времени прошло с предыдущего кадра
            float dt = deltaClock.restart().asSeconds();

            // Настраиваем громкость фоновой музыки в зависимости от включённого звука
            backgroundMusic.setVolume(soundOn ? 50 : 0);

            // Если игра в стадии загрузки, обновляем заставку
            if (state == LOADING) {
                loadingScreen.update(dt);

                // После завершения загрузки и небольшой задержки переходим в главное меню
                if (loadingScreen.completed && loadingClock.getElapsedTime().asSeconds() > 0.5f) {
                    state = MAIN_MENU;
                }
            }
            // Если отображается главное меню — обновляем его анимацию
            else if (state == MAIN_MENU) {
                mainMenu.updateAnimation(dt);
            }

            // Обрабатываем пользовательский ввод (мышь, клавиши и пр.)
            handleEvents();

            // Игровое состояние — только в режиме игры, если не на паузе
            if (state == GAME_PLAY && !gamePaused) {
                update(dt); // логика клиента, приготовлений и т.п.
            }
            // Проверяем наведена ли мышь на финальную кнопку
            else if (state == FINAL_SCREEN) {
                Vector2i mousePos = Mouse::getPosition(window);
                finalButtonHovered = finalButtonBounds.contains(static_cast<Vector2f>(mousePos));
                finalButtonSprite.setTexture(finalButtonHovered ? finalButtonHover : finalButtonNormal);
            }
            // Наведение мыши на кнопку в экране результатов уровня
            else if (state == LEVEL_RESULTS) {
                Vector2i mousePos = Mouse::getPosition(window);
                levelResultsScreen.update(static_cast<Vector2f>(mousePos));
            }

            // Отрисовываем кадр
            draw();
        }
    }

    // Обработка всех пользовательских событий: клики, движение мыши, клавиатура
    void handleEvents() {
        Event event;
        while (window.pollEvent(event)) {

            // Выход из игры по нажатию на крестик окна
            if (event.type == Event::Closed) {
                window.close();
            }

            // Вычисляем координаты мыши при разных типах событий
            Vector2f mousePos(0, 0);
            if (event.type == Event::MouseMoved) {
                mousePos = Vector2f(static_cast<float>(event.mouseMove.x),
                    static_cast<float>(event.mouseMove.y));
            }
            else if (event.type == Event::MouseButtonPressed ||
                event.type == Event::MouseButtonReleased) {
                mousePos = Vector2f(static_cast<float>(event.mouseButton.x),
                    static_cast<float>(event.mouseButton.y));
            }

            if (state == MAIN_MENU) {
                // Наведение на кнопки
                if (event.type == Event::MouseMoved) {
                    mainMenu.update(mousePos.x, mousePos.y);
                }

                // Клик по кнопкам
                if (event.type == Event::MouseButtonPressed &&
                    event.mouseButton.button == Mouse::Left) {

                    int action = mainMenu.handleClick(mousePos.x, mousePos.y);
                    if (action == 0) {
                        // Новая игра
                        saveManager.resetProgress();
                        currentLevelNumber = 1;
                        state = LEVEL_SELECT;
                        if (levelSelectMenu) delete levelSelectMenu;
                        levelSelectMenu = new LevelSelectMenu(&saveManager, &soundOn);
                    }
                    else if (action == 1) {
                        // Продолжить игру
                        state = LEVEL_SELECT;
                        if (levelSelectMenu) delete levelSelectMenu;
                        levelSelectMenu = new LevelSelectMenu(&saveManager, &soundOn);
                    }
                    else if (action == 2) {
                        // Выход из игры (сохраняем текущий уровень)
                        saveManager.setCurrentLevel(currentLevelNumber + 1);
                        window.close();
                    }
                }

                // Esc — закрыть окно «о программе», если открыто
                if (event.type == Event::KeyPressed &&
                    event.key.code == Keyboard::Escape) {
                    if (mainMenu.aboutVisible) mainMenu.aboutVisible = false;
                }

                helpMenuHovered = helpMenuBounds.contains(mousePos);
                helpMenuText.setFillColor(helpMenuHovered ? Color(200, 200, 255) : Color::White);

                // Обработка клика по меню "Справка"
                if (event.type == Event::MouseButtonPressed &&
                    event.mouseButton.button == Mouse::Left &&
                    helpMenuHovered) {
#ifdef _WIN32
                    ShellExecuteW(0, 0, L"Справка.chm", 0, 0, SW_SHOW);
#else
                    system("start Справка.chm");
#endif
                }
            }
            else if (state == LEVEL_SELECT && levelSelectMenu) {
                // Наведение мыши — обновление иконок уровней
                if (event.type == Event::MouseMoved) {
                    levelSelectMenu->update(mousePos.x, mousePos.y);
                }

                // Клик по кнопкам
                if (event.type == Event::MouseButtonPressed &&
                    event.mouseButton.button == Mouse::Left) {

                    int action = levelSelectMenu->handleClick(mousePos.x, mousePos.y);

                    if (action == 0) {
                        // Кнопка "Назад" — возвращаемся в главное меню
                        state = MAIN_MENU;
                        mainMenu.reload();
                    }
                    else if (action > 0) {
                        // Выбор уровня
                        currentLevelNumber = action;
                        saveManager.setCurrentLevel(currentLevelNumber);
                        state = GAME_PLAY;

                        resetLevel();         // сбрасываем состояние игры
                        levelElapsedTime = 0.0f;
                        spawnClient();        // запускаем первого клиента
                    }
                }

                // Esc — возвращение в главное меню
                if (event.type == Event::KeyPressed &&
                    event.key.code == Keyboard::Escape) {
                    state = MAIN_MENU;
                    mainMenu.reload();
                }
            }
            else if (state == GAME_PLAY) {
                if (event.type == Event::MouseMoved) {
                    Vector2f mousePos = static_cast<Vector2f>(Mouse::getPosition(window));

                    if (!gamePaused) {
                        pauseButtonHovered = pauseButtonSprite.getGlobalBounds().contains(mousePos);
                        pauseButtonSprite.setTexture(pauseButtonHovered ? pauseButtonHoverTex : pauseButtonNormalTex);
                    }
                    else {
                        playButtonHovered = playButtonBounds.contains(mousePos);
                        playButtonSprite.setTexture(playButtonHovered ? playButtonHoverTex : playButtonNormalTex);

                        mainMenuButtonHovered = mainMenuButtonBounds.contains(mousePos);
                        mainMenuButtonSprite.setTexture(mainMenuButtonHovered ? mainMenuButtonHoverTex : mainMenuButtonNormalTex);
                    }
                }

                if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    Vector2f mousePos = static_cast<Vector2f>(Mouse::getPosition(window));

                    // Нажатие на кнопку паузы
                    if (!gamePaused && pauseButtonSprite.getGlobalBounds().contains(mousePos)) {
                        gamePaused = true;
                        if (soundOn) {
                            clickSound.play();
                        }
                        continue;
                    }


                    // Обработка кнопок в меню паузы
                    else if (gamePaused) {
                        if (playButtonBounds.contains(mousePos)) {
                            gamePaused = false;
                            if (soundOn) {
                                clickSound.play();
                            }
                        }
                        else if (mainMenuButtonBounds.contains(mousePos)) {
                            saveManager.setCurrentLevel(currentLevelNumber);
                            state = MAIN_MENU;
                            resetLevel();
                            gamePaused = false;
                            if (soundOn) {
                                clickSound.play();
                            }
                        }
                        else if (gameSoundButton.contains(mousePos)) {
                            gameSoundButton.toggle();
                            if (soundOn) {
                                clickSound.play();
                            }
                        }
                        continue;
                    }

                    // Обработка игрового процесса
                    if (!gamePaused) {
                        if (croissantArea.contains(mousePos) && croissantDish == nullptr) {
                            croissantDish = new CroissantDish(Vector2f(657, 371));
                        }
                        else if (sandwichArea.contains(mousePos) && sandwichDish == nullptr) {
                            sandwichDish = new SandwichDish(Vector2f(532, 362));
                        }

                        if (sausageArea.contains(mousePos)) {
                            activeIngredients.push_back(DraggableItem("sausage", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }
                        else if (pepperArea.contains(mousePos)) {
                            activeIngredients.push_back(DraggableItem("pepper", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }
                        else if (cheeseArea.contains(mousePos)) {
                            activeIngredients.push_back(DraggableItem("cheese", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }
                        else if (bananaArea.contains(mousePos)) {
                            activeIngredients.push_back(DraggableItem("banana", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }
                        else if (chocolateArea.contains(mousePos)) {
                            activeIngredients.push_back(DraggableItem("chocolate", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }
                        else if (coffeeArea.contains(mousePos) && coffeeMachine->state == Machine::Finished) {
                            activeIngredients.push_back(DraggableItem("coffee", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }
                        else if (cake1Area.contains(mousePos)) {
                            activeIngredients.push_back(DraggableItem("cake1", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }
                        else if (cake2Area.contains(mousePos)) {
                            activeIngredients.push_back(DraggableItem("cake2", mousePos));
                            currentDragItem = &activeIngredients.back();
                            currentDragItem->startDrag(mousePos);
                        }

                        if (croissantDish) {
                            croissantDish->startDrag(mousePos);
                            if (croissantDish->isDragging) {
                                currentDragCroissant = croissantDish;
                            }
                        }
                        if (sandwichDish) {
                            sandwichDish->startDrag(mousePos);
                            if (sandwichDish->isDragging) {
                                currentDragSandwich = sandwichDish;
                                sandwichDish->dragOffset = sandwichDish->sprite.getPosition() - mousePos;
                            }
                        }

                        if (gameSoundButton.contains(mousePos)) {
                            gameSoundButton.toggle();
                        }
                        else if (coffeeMachine->isClicked(mousePos)) {
                            if (coffeeMachine->state == Machine::Finished) {
                                coffeeMachine->reset();

                                // Создаем предмет кофе
                                activeIngredients.push_back(DraggableItem("coffee", mousePos));
                                currentDragItem = &activeIngredients.back();
                                currentDragItem->startDrag(mousePos);
                            }
                            else if (coffeeMachine->state == Machine::Idle) {
                                prepareCoffee();
                            }
                        }
                        else if (dialogCloud.contains(mousePos)) {
                            dialogCloud.handleClick();
                        }
                    }

                }
                else if (event.type == Event::MouseButtonReleased && event.mouseButton.button == Mouse::Left) {
                    if (!gamePaused) {
                        bool servedToClient = false;

                        if (trashArea.contains(mousePos)) {
                            if (currentDragItem) {
                                for (auto it = activeIngredients.begin(); it != activeIngredients.end(); ) {
                                    if (&(*it) == currentDragItem) {
                                        it = activeIngredients.erase(it);
                                        break;
                                    }
                                    else {
                                        ++it;
                                    }
                                }
                                currentDragItem = nullptr;
                            }
                            else if (currentDragCroissant) {
                                delete croissantDish;
                                croissantDish = nullptr;
                                currentDragCroissant = nullptr;
                            }
                            else if (currentDragSandwich) {
                                delete sandwichDish;
                                sandwichDish = nullptr;
                                currentDragSandwich = nullptr;
                            }
                        }
                        else {
                            if (currentDragItem) {
                                if (currentDragItem->type == "coffee") {
                                    if (currentClient && currentClient->sprite.getGlobalBounds().contains(mousePos)) {
                                        bool served = currentClient->serveDish("coffee");
                                        if (served) {
                                            servedToClient = true;
                                            orderWindow.setServedOrders(currentClient->servedOrders);
                                            serveOrder();
                                            for (auto it = activeIngredients.begin(); it != activeIngredients.end();) {
                                                if (&(*it) == currentDragItem) {
                                                    it = activeIngredients.erase(it);
                                                    break;
                                                }
                                                else {
                                                    ++it;
                                                }
                                            }
                                        }
                                        else {
                                            currentClient->failOrder();
                                        }
                                    }

                                    if (!servedToClient) {
                                        currentDragItem->resetPosition();
                                    }

                                    for (auto it = activeIngredients.begin(); it != activeIngredients.end(); ++it) {
                                        if (&(*it) == currentDragItem) {
                                            activeIngredients.erase(it);
                                            break;
                                        }
                                    }
                                }
                                else if (currentDragItem->type == "cake1" || currentDragItem->type == "cake2") {
                                    if (currentClient && currentClient->sprite.getGlobalBounds().contains(mousePos)) {
                                        bool served = currentClient->serveDish(currentDragItem->type);
                                        if (served) {
                                            servedToClient = true;
                                            orderWindow.setServedOrders(currentClient->servedOrders);
                                            serveOrder();
                                        }
                                        else {
                                            currentClient->failOrder();
                                        }
                                    }

                                    if (!servedToClient) {
                                        currentDragItem->resetPosition();
                                    }

                                    for (auto it = activeIngredients.begin(); it != activeIngredients.end(); ++it) {
                                        if (&(*it) == currentDragItem) {
                                            activeIngredients.erase(it);
                                            break;
                                        }
                                    }
                                }
                                else {
                                    bool addedToDish = false;
                                    if (croissantDish && croissantDish->sprite.getGlobalBounds().contains(mousePos)) {
                                        croissantDish->addIngredient(currentDragItem->type);
                                        addedToDish = true;
                                    }
                                    if (sandwichDish && sandwichDish->sprite.getGlobalBounds().contains(mousePos)) {
                                        sandwichDish->addIngredient(currentDragItem->type);
                                        addedToDish = true;
                                    }

                                    if (currentDragItem->isStatic && addedToDish) {
                                        currentDragItem->setInactive();
                                    }
                                    else if (!addedToDish) {
                                        currentDragItem->resetPosition();
                                    }

                                    if (!currentDragItem->isStatic) {
                                        for (auto it = activeIngredients.begin(); it != activeIngredients.end(); ) {
                                            if (&(*it) == currentDragItem) {
                                                it = activeIngredients.erase(it);
                                                break;
                                            }
                                            else {
                                                ++it;
                                            }
                                        }
                                    }
                                }
                                currentDragItem = nullptr;
                            }

                            if (currentDragCroissant) {
                                if (currentClient && currentClient->sprite.getGlobalBounds().contains(mousePos)) {
                                    string dishType;
                                    switch (croissantDish->currentState) {
                                    case CroissantDish::CHOCOLATE: dishType = "croissant_chocolate"; break;
                                    case CroissantDish::BANANA: dishType = "croissant_banana"; break;
                                    case CroissantDish::BOTH: dishType = "croissant_both"; break;
                                    default: dishType = "croissant_plain"; break;
                                    }

                                    bool served = currentClient->serveDish(dishType);
                                    if (served) {
                                        orderWindow.setServedOrders(currentClient->servedOrders);
                                        delete croissantDish;
                                        croissantDish = nullptr;
                                        serveOrder();
                                    }
                                    else {
                                        currentClient->failOrder();
                                        currentDragCroissant->resetPosition();
                                    }
                                }
                                else {
                                    currentDragCroissant->resetPosition();
                                }
                                currentDragCroissant->stopDrag();
                                currentDragCroissant = nullptr;
                            }

                            if (currentDragSandwich) {
                                if (currentClient && currentClient->sprite.getGlobalBounds().contains(mousePos)) {
                                    string dishType;
                                    if (sandwichDish->hasSausage && sandwichDish->hasCheese && sandwichDish->hasPepper)
                                        dishType = "sandwich_all";
                                    else if (sandwichDish->hasSausage && sandwichDish->hasCheese)
                                        dishType = "sandwich_sausage_cheese";
                                    else if (sandwichDish->hasSausage && sandwichDish->hasPepper)
                                        dishType = "sandwich_sausage_pepper";
                                    else if (sandwichDish->hasCheese && sandwichDish->hasPepper)
                                        dishType = "sandwich_cheese_pepper";
                                    else if (sandwichDish->hasSausage)
                                        dishType = "sandwich_sausage";
                                    else if (sandwichDish->hasCheese)
                                        dishType = "sandwich_cheese";
                                    else if (sandwichDish->hasPepper)
                                        dishType = "sandwich_pepper";
                                    else
                                        dishType = "sandwich_plain";

                                    bool served = currentClient->serveDish(dishType);
                                    if (served) {
                                        orderWindow.setServedOrders(currentClient->servedOrders);
                                        delete sandwichDish;
                                        sandwichDish = nullptr;
                                        serveOrder();
                                    }
                                    else {
                                        currentClient->failOrder();
                                        currentDragSandwich->resetPosition();
                                    }
                                }
                                else {
                                    currentDragSandwich->resetPosition();
                                }
                                currentDragSandwich->stopDrag();
                                currentDragSandwich = nullptr;
                            }
                        }
                    }
                }
                else if (event.type == Event::MouseMoved) {
                    if (!gamePaused) {
                        if (currentDragItem) {
                            currentDragItem->drag(mousePos);
                        }
                        if (currentDragCroissant) {
                            currentDragCroissant->drag(mousePos);
                        }
                        if (currentDragSandwich) {
                            currentDragSandwich->drag(mousePos);
                        }
                        gameSoundButton.update(mousePos);
                    }
                }
                else if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
                    gamePaused = !gamePaused;
                    if (soundOn && gamePaused) {
                        clickSound.play();
                    }
                }

            }
            else if (state == FINAL_SCREEN) {
                if (event.type == Event::MouseButtonPressed) {
                    // Любой клик возвращает в главное меню
                    if (soundOn) {
                            clickSound.play();
                        }
                        state = MAIN_MENU;
                        mainMenu.reload();
                    }
                }
            else if (state == LEVEL_RESULTS) {
                if (event.type == Event::MouseMoved) {
                    Vector2f mousePos(static_cast<float>(event.mouseMove.x),
                        static_cast<float>(event.mouseMove.y));
                    levelResultsScreen.update(mousePos);
                }
                else if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                    Vector2f mousePos(static_cast<float>(event.mouseButton.x),
                        static_cast<float>(event.mouseButton.y));
                    if (levelResultsScreen.buttonBounds.contains(mousePos)) {
                        if (soundOn) {
                            clickSound.play();
                        }
                        // Если это был финальный уровень - переходим к финальному экрану
                        if (isFinalLevelCompleted) {
                            state = FINAL_SCREEN;
                        }
                        else {
                            state = LEVEL_SELECT;
                            if (levelSelectMenu) delete levelSelectMenu;
                            levelSelectMenu = new LevelSelectMenu(&saveManager, &soundOn);
                        }
                    }
                }
                else if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
                    state = LEVEL_SELECT;
                    if (levelSelectMenu) delete levelSelectMenu;
                    levelSelectMenu = new LevelSelectMenu(&saveManager, &soundOn);
                }
            }
        }
    }

    void update(float dt) {
        backgroundMusic.setVolume(soundOn ? 50 : 0);
        if (state == GAME_PLAY && !gamePaused) {
            gameSoundButton.updateState();
            levelElapsedTime += dt;

            if (currentClient) {
                currentClient->update(dt, gamePaused);
            }

            if (dialogCloud.completed && currentClient && currentClient->state == Client::StoryTelling) {
                currentClient->state = Client::Leaving;
                currentClient->currentFrame = currentClient->frameCount - 1;
            }
            if (currentClient && currentClient->state == Client::Waiting) {
                orderWindow.visible = true;
            }
            else if (currentClient && currentClient->isGone) {
                float clientTime = levelElapsedTime - currentClientSpawnTime;
                clientTimes.push_back(clientTime);

                delete currentClient;
                currentClient = nullptr;
                clientsServed++;

                if (clientsServed >= 3) {
                    float totalTime = levelElapsedTime;

                    float avgTime = 0.0f;
                    if (!clientTimes.empty()) {
                        for (float t : clientTimes)
                            avgTime += t;
                        avgTime /= clientTimes.size();
                    }

                    levelResultsScreen.setResults(
                        currentLevelNumber,
                        successfulServed,
                        3,
                        totalTime
                    );

                    // Всегда показываем экран результатов
                    state = LEVEL_RESULTS;

                    // Разблокируем следующий уровень, если это не 6 уровень
                    if (successfulServed >= 1 && currentLevelNumber < 6) {
                        saveManager.unlockLevel(currentLevelNumber + 1);
                    }

                    // Для 6 уровня переходим сразу к финалу после результатов
                    if (currentLevelNumber == 6) {
                        // Запомним что это финальный уровень
                        isFinalLevelCompleted = true;
                    }
                }
                else {
                    spawningClient = true;
                    clientSpawnTimer.restart();
                }

                orderWindow.visible = false;
                dialogCloud.visible = false;
                dialogCloud.completed = true;
            }
            else if (spawningClient) {
                if (clientSpawnTimer.getElapsedTime().asSeconds() > 2.0f) {
                    spawnClient();
                    spawningClient = false;
                }
            }
            coffeeMachine->update(dt, gamePaused);
            dialogCloud.update(dt);
            dialogCloud.textOffset = Vector2f(120, 50);
            dialogCloud.maxTextWidth = 350;
        }
    }

    void draw() {
        // Очищаем окно чёрным цветом перед новой отрисовкой
        window.clear(Color::Black);

        // Заставка загрузки
        if (state == LOADING) {
            loadingScreen.draw(window);
        }

        // Главное меню
        else if (state == MAIN_MENU) {
            mainMenu.draw(window);
            window.draw(menuBar);
            window.draw(helpMenuText); // Добавьте эту строку
        }

        // Меню выбора уровня
        else if (state == LEVEL_SELECT && levelSelectMenu) {
            levelSelectMenu->draw(window);
        }

        // Игровой процесс
        else if (state == GAME_PLAY) {
            // Отрисовка фона уровня и прилавка
            window.draw(levelBgSprite);
            window.draw(counterSprite);

            // Отрисовка активных блюд
            if (croissantDish) window.draw(croissantDish->sprite);
            if (sandwichDish) window.draw(sandwichDish->sprite);

            // Отрисовка всех активных ингредиентов на экране
            for (auto& ingredient : activeIngredients) {
                window.draw(ingredient.sprite);
            }

            // Отрисовка кофе-машины и прогресса приготовления
            coffeeMachine->draw(window);

            // Отрисовка клиента, если он есть
            if (currentClient) {
                window.draw(currentClient->sprite);

                // Имя клиента над его головой
                Text nameText;
                nameText.setFont(font);
                nameText.setString(currentClient->name);
                nameText.setCharacterSize(24);
                nameText.setFillColor(Color(139, 69, 19));           // коричневый цвет
                nameText.setOutlineColor(Color::White);              // белая обводка
                nameText.setOutlineThickness(2);

                FloatRect clientBounds = currentClient->sprite.getGlobalBounds();
                nameText.setPosition(
                    clientBounds.left + clientBounds.width / 2 - nameText.getLocalBounds().width / 2,
                    clientBounds.top - 20
                );

                window.draw(nameText);
            }

            // Текстовая информация: номер уровня и сколько клиентов обслужено
            Text infoText;
            infoText.setFont(font);
            infoText.setString(L"Уровень: " + to_wstring(currentLevelNumber) +
                L"\nКлиентов: " + to_wstring(clientsServed) + L"/3");
            infoText.setCharacterSize(24);
            infoText.setFillColor(Color(139, 69, 19));
            infoText.setOutlineColor(Color::White);
            infoText.setOutlineThickness(2);
            infoText.setPosition(50, 50);
            window.draw(infoText);

            // Таймер ожидания клиента
            if (currentClient && currentClient->state == Client::Waiting) {
                float remainingTime = currentClient->patienceTime - currentClient->elapsedPatience;
                if (remainingTime < 0) remainingTime = 0;

                Text timerText;
                timerText.setFont(font);
                timerText.setString(L"Время: " + to_wstring(static_cast<int>(remainingTime)));
                timerText.setCharacterSize(24);
                timerText.setFillColor(Color(139, 69, 19));
                timerText.setOutlineColor(Color::White);
                timerText.setOutlineThickness(2);
                timerText.setOrigin(timerText.getLocalBounds().width / 2.f, 0.f);
                timerText.setPosition(640, 10); // центр верхней части экрана
                window.draw(timerText);
            }

            // Отображение окна с заказом клиента
            orderWindow.draw(window);

            // Облако диалога (если активно)
            dialogCloud.draw(window);

            // Кнопка управления звуком
            gameSoundButton.draw(window);

            // Кнопка паузы в правом верхнем углу
            pauseButtonSprite.setTexture(pauseButtonHovered ? pauseButtonHoverTex : pauseButtonNormalTex);
            window.draw(pauseButtonSprite);

            // Отрисовка элементов при активной паузе
            if (gamePaused) {
                window.draw(pauseOverlaySprite);        // полупрозрачный тёмный фон

                playButtonSprite.setTexture(playButtonHovered ? playButtonHoverTex : playButtonNormalTex);
                window.draw(playButtonSprite);

                mainMenuButtonSprite.setTexture(mainMenuButtonHovered ? mainMenuButtonHoverTex : mainMenuButtonNormalTex);
                window.draw(mainMenuButtonSprite);

                gameSoundButton.draw(window); // кнопка звука также доступна во время паузы
            }
            // Рисуем перетаскиваемые объекты поверх всего
            if (currentDragItem) {
                window.draw(currentDragItem->sprite);
            }
            if (currentDragCroissant) {
                window.draw(currentDragCroissant->sprite);
            }
            if (currentDragSandwich) {
                window.draw(currentDragSandwich->sprite);
            }
        }

        // Финальный экран после прохождения всех уровней
        else if (state == FINAL_SCREEN) {
            window.draw(finalScreenSprite);
        }

        // Экран результатов уровня
        else if (state == LEVEL_RESULTS) {
            levelResultsScreen.draw(window);
        }

        // Отображаем всё, что нарисовано
        window.display();
    }
};

int main() {
    setlocale(LC_ALL, "rus");
    srand(time(0));
    Game game;
    game.run();
    return 0;
}