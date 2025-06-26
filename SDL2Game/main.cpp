#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

using namespace std;

struct Toy {
    string name;
    string description;
    float price;
    int quantity;
};

SDL_Texture* RenderText(SDL_Renderer* renderer, TTF_Font* font, const string& text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        cerr << "TTF_RenderUTF8_Blended Error: " << TTF_GetError() << endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

enum class AppState {
    MENU,
    STORE,
    EDIT,
    EXIT
};

void RenderRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, SDL_Color color, int radius) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect fillRect = { rect.x + radius, rect.y, rect.w - 2 * radius, rect.h };
    SDL_RenderFillRect(renderer, &fillRect);
    fillRect = { rect.x, rect.y + radius, rect.w, rect.h - 2 * radius };
    SDL_RenderFillRect(renderer, &fillRect);

    for (int w = 0; w < radius; ++w) {
        int height = static_cast<int>(sqrt(radius * radius - (radius - w) * (radius - w)));
        for (int h = 0; h < height; ++h) {
            SDL_RenderDrawPoint(renderer, rect.x + w, rect.y + h);
            SDL_RenderDrawPoint(renderer, rect.x + rect.w - 1 - w, rect.y + h);
            SDL_RenderDrawPoint(renderer, rect.x + w, rect.y + rect.h - 1 - h);
            SDL_RenderDrawPoint(renderer, rect.x + rect.w - 1 - w, rect.y + rect.h - 1 - h);
        }
    }
}

bool IsPointInRect(int px, int py, const SDL_Rect& rect) {
    return px >= rect.x && px < rect.x + rect.w && py >= rect.y && py < rect.y + rect.h;
}

void TruncateUTF8(std::string& str, size_t maxLen) {
    if (str.size() > maxLen) {
        str = str.substr(0, maxLen);
    }
}

void DrawCursor(SDL_Renderer* renderer, int x, int y, int h, Uint32 ticks) {
    const Uint32 blinkTime = 500;
    if ((ticks / blinkTime) % 2 == 0) {
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_RenderDrawLine(renderer, x, y, x, y + h);
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cerr << "SDL initialization error: " << SDL_GetError() << endl;
        return 1;
    }

    if (TTF_Init() == -1) {
        cerr << "SDL_ttf initialization error: " << TTF_GetError() << endl;
        SDL_Quit();
        return 1;
    }

    int winWidth = 800;
    int winHeight = 600;

    SDL_Window* window = SDL_CreateWindow("Toy Store",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        winWidth, winHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    if (!window) {
        cerr << "Window creation error: " << SDL_GetError() << endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        cerr << "Renderer creation error: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    const char* fontPath = "C:\\Windows\\Fonts\\Bahnschrift.ttf";
    TTF_Font* font = TTF_OpenFont(fontPath, 24);
    if (!font) {
        cerr << "Font loading error: " << TTF_GetError() << endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    vector<Toy> store = {
        {"Lego Set", "A fun building set for kids.", 29.99f, 10},
        {"Doll", "A beautiful doll for imaginative play.", 19.99f, 5},
        {"Toy Car", "A speedy little car for racing.", 9.99f, 15}
    };

    float balance = 0.0f;
    AppState state = AppState::MENU;
    bool running = true;
    SDL_Event event;

    int menuSelectedIndex = 0;
    int storeSelectedIndex = 0;

    string editName;
    string editDescription;
    string editPriceStr;
    int editFocusedField = 0;

    SDL_Color bgMenuColor = { 30, 30, 60, 255 };
    SDL_Color bgStoreColor = { 50, 50, 80, 255 };

    SDL_Color baseTextColor = { 230, 230, 230, 255 };
    SDL_Color highlightColorLight = { 255, 180, 180, 255 };
    SDL_Color highlightColorDark = { 255, 120, 120, 255 };

    Uint32 startTicks = SDL_GetTicks();

    SDL_Rect menuPlayButton;
    SDL_Rect menuExitButton;

    SDL_Rect btnUp;
    SDL_Rect btnDown;
    SDL_Rect btnAdd;
    SDL_Rect btnDelete;
    SDL_Rect btnSell;
    SDL_Rect btnBack;
    SDL_Rect btnEdit;

    SDL_StartTextInput();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                winWidth = event.window.data1;
                winHeight = event.window.data2;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mx = event.button.x;
                int my = event.button.y;

                if (state == AppState::MENU) {
                    if (IsPointInRect(mx, my, menuPlayButton)) {
                        state = AppState::STORE;
                        storeSelectedIndex = 0;
                    }
                    else if (IsPointInRect(mx, my, menuExitButton)) {
                        running = false;
                    }
                }
                else if (state == AppState::STORE) {
                    if (IsPointInRect(mx, my, btnUp)) {
                        if (storeSelectedIndex > 0) storeSelectedIndex--;
                    }
                    else if (IsPointInRect(mx, my, btnDown)) {
                        if (storeSelectedIndex < int(store.size()) - 1) storeSelectedIndex++;
                    }
                    else if (IsPointInRect(mx, my, btnAdd)) {
                        store.push_back({ "New Toy", "A newly added toy.", 14.99f, 7 });
                        storeSelectedIndex = int(store.size()) - 1;
                    }
                    else if (IsPointInRect(mx, my, btnDelete)) {
                        if (!store.empty() && storeSelectedIndex < int(store.size())) {
                            store.erase(store.begin() + storeSelectedIndex);
                            if (storeSelectedIndex > 0) storeSelectedIndex--;
                        }
                    }
                    else if (IsPointInRect(mx, my, btnSell)) {
                        if (!store.empty() && storeSelectedIndex < int(store.size())) {
                            if (store[storeSelectedIndex].quantity > 0) {
                                if (store[storeSelectedIndex].quantity == 1) {
                                    balance += store[storeSelectedIndex].price;
                                    store.erase(store.begin() + storeSelectedIndex);
                                    if (storeSelectedIndex >= int(store.size())) {
                                        storeSelectedIndex = int(store.size()) - 1;
                                    }
                                }
                                else {
                                    store[storeSelectedIndex].quantity--;
                                    balance += store[storeSelectedIndex].price;
                                }
                            }
                        }
                    }
                    else if (IsPointInRect(mx, my, btnEdit)) {
                        if (storeSelectedIndex >= 0 && storeSelectedIndex < (int)store.size()) {
                            editName = store[storeSelectedIndex].name;
                            editDescription = store[storeSelectedIndex].description;
                            editPriceStr = to_string(store[storeSelectedIndex].price);
                            editPriceStr.erase(editPriceStr.find_last_not_of('0') + 1, std::string::npos);
                            if (editPriceStr.back() == '.') editPriceStr.pop_back();
                            editFocusedField = 0;
                            state = AppState::EDIT;
                        }
                    }
                    else if (IsPointInRect(mx, my, btnBack)) {
                        state = AppState::MENU;
                    }
                    else {
                        int lineHeight = winHeight / 12;
                        int boxHeight = lineHeight * 2 / 3;
                        int boxWidth = winWidth - 100;
                        int xPosition = 50;
                        int startY = winHeight / 10;
                        for (size_t i = 0; i < store.size(); ++i) {
                            SDL_Rect itemRect = { xPosition, startY + int(i) * lineHeight, boxWidth, boxHeight };
                            if (IsPointInRect(mx, my, itemRect)) {
                                storeSelectedIndex = int(i);
                                break;
                            }
                        }
                    }
                }
                else if (state == AppState::EDIT) {
                    int lineHeight = 40;
                    int inputFieldHeight = 36;
                    int marginTop = winHeight / 5;
                    int inputWidth = winWidth - 100;

                    SDL_Rect nameRect = { 50, marginTop, inputWidth, inputFieldHeight };
                    SDL_Rect priceRect = { 50, marginTop + (lineHeight * 2), inputWidth, inputFieldHeight };
                    SDL_Rect descRect = { 50, marginTop + (lineHeight * 4), inputWidth, inputFieldHeight * 3 };

                    int btnWidth = 150;
                    int btnHeight = 50;
                    int btnY = winHeight - 80;
                    SDL_Rect btnSave = { winWidth / 2 - btnWidth - 20, btnY, btnWidth, btnHeight };
                    SDL_Rect btnCancel = { winWidth / 2 + 20, btnY, btnWidth, btnHeight };

                    if (IsPointInRect(mx, my, nameRect)) editFocusedField = 0;
                    else if (IsPointInRect(mx, my, priceRect)) editFocusedField = 1;
                    else if (IsPointInRect(mx, my, descRect)) editFocusedField = 2;
                    else if (IsPointInRect(mx, my, btnSave)) {
                        if (storeSelectedIndex >= 0 && storeSelectedIndex < (int)store.size()) {
                            store[storeSelectedIndex].name = editName;
                            store[storeSelectedIndex].description = editDescription;
                            try {
                                store[storeSelectedIndex].price = stof(editPriceStr);
                            }
                            catch (...) {
                            }
                        }
                        state = AppState::STORE;
                    }
                    else if (IsPointInRect(mx, my, btnCancel)) {
                        state = AppState::STORE;
                    }
                }
            }
            else if (event.type == SDL_TEXTINPUT && state == AppState::EDIT) {
                string* currentField = nullptr;
                if (editFocusedField == 0) currentField = &editName;
                else if (editFocusedField == 1) currentField = &editPriceStr;
                else if (editFocusedField == 2) currentField = &editDescription;

                if (currentField) {
                    if (currentField->length() + strlen(event.text.text) < 256) {
                        if (editFocusedField == 1) {
                            for (size_t i = 0; i < strlen(event.text.text); ++i) {
                                char c = event.text.text[i];
                                if (!(isdigit(c) || c == '.' || c == ',')) {
                                    continue;
                                }
                                currentField->push_back(c);
                            }
                        }
                        else {
                            currentField->append(event.text.text);
                        }
                    }
                }
            }
            else if (event.type == SDL_KEYDOWN && state == AppState::EDIT) {
                string* currentField = nullptr;
                if (editFocusedField == 0) currentField = &editName;
                else if (editFocusedField == 1) currentField = &editPriceStr;
                else if (editFocusedField == 2) currentField = &editDescription;

                if (event.key.keysym.sym == SDLK_BACKSPACE && currentField && !currentField->empty()) {
                    currentField->pop_back();
                }
                else if (event.key.keysym.sym == SDLK_TAB) {
                    editFocusedField = (editFocusedField + 1) % 3;
                }
                else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (editFocusedField < 2) {
                        editFocusedField++;
                    }
                    else {
                        if (storeSelectedIndex >= 0 && storeSelectedIndex < (int)store.size()) {
                            store[storeSelectedIndex].name = editName;
                            store[storeSelectedIndex].description = editDescription;
                            try {
                                store[storeSelectedIndex].price = stof(editPriceStr);
                            }
                            catch (...) {
                            }
                        }
                        state = AppState::STORE;
                    }
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    state = AppState::STORE;
                }
            }
        }

        int btnWidth = winWidth / 3;
        int btnHeight = winHeight / 10;
        int btnX = (winWidth - btnWidth) / 2;

        menuPlayButton = { btnX, winHeight / 3, btnWidth, btnHeight };
        menuExitButton = { btnX, winHeight / 3 + btnHeight + 20, btnWidth, btnHeight };

        int sBtnWidth = (winWidth - 90) / 7;
        int sBtnHeight = 50;
        int sBtnY = winHeight - sBtnHeight - 20;

        btnUp = { 10, sBtnY, sBtnWidth, sBtnHeight };
        btnDown = { 20 + sBtnWidth, sBtnY, sBtnWidth, sBtnHeight };
        btnAdd = { 30 + sBtnWidth * 2, sBtnY, sBtnWidth, sBtnHeight };
        btnDelete = { 40 + sBtnWidth * 3, sBtnY, sBtnWidth, sBtnHeight };
        btnSell = { 50 + sBtnWidth * 4, sBtnY, sBtnWidth, sBtnHeight };
        btnEdit = { 60 + sBtnWidth * 5, sBtnY, sBtnWidth, sBtnHeight };
        btnBack = { 70 + sBtnWidth * 6, sBtnY, sBtnWidth, sBtnHeight };

        Uint32 elapsed = SDL_GetTicks() - startTicks;
        float t = (elapsed % 2000) / 2000.f;
        float pulse = (sin(t * 2.f * 3.14159f) + 1.f) / 2.f;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (state == AppState::MENU) {
            SDL_SetRenderDrawColor(renderer, bgMenuColor.r, bgMenuColor.g, bgMenuColor.b, bgMenuColor.a);
            SDL_RenderClear(renderer);

            auto DrawButton = [&](SDL_Rect rect, const string& label, bool hovered) {
                SDL_Color baseColor = hovered ? SDL_Color{ 255,180,180,220 } : SDL_Color{ 60,60,90,160 };
                RenderRoundedRect(renderer, rect, baseColor, 12);
                SDL_Texture* textTex = RenderText(renderer, font, label, baseTextColor);
                if (textTex) {
                    int w, h;
                    SDL_QueryTexture(textTex, nullptr, nullptr, &w, &h);
                    SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
                    SDL_RenderCopy(renderer, textTex, nullptr, &textRect);
                    SDL_DestroyTexture(textTex);
                }
                };

            int mx, my;
            SDL_GetMouseState(&mx, &my);

            DrawButton(menuPlayButton, "Play", IsPointInRect(mx, my, menuPlayButton));
            DrawButton(menuExitButton, "Exit", IsPointInRect(mx, my, menuExitButton));

            SDL_RenderPresent(renderer);
        }
        else if (state == AppState::STORE) {
            SDL_SetRenderDrawColor(renderer, bgStoreColor.r, bgStoreColor.g, bgStoreColor.b, bgStoreColor.a);
            SDL_RenderClear(renderer);

            int lineHeight = winHeight / 12;
            int boxHeight = lineHeight * 2 / 3;
            int boxWidth = winWidth - 100;
            int xPosition = 50;
            int startY = winHeight / 10;

            for (size_t i = 0; i < store.size(); ++i) {
                SDL_Color boxColor;
                if (int(i) == storeSelectedIndex) {
                    Uint8 r = Uint8(highlightColorDark.r * (1.f - pulse) + highlightColorLight.r * pulse);
                    Uint8 g = Uint8(highlightColorDark.g * (1.f - pulse) + highlightColorLight.g * pulse);
                    Uint8 b = Uint8(highlightColorDark.b * (1.f - pulse) + highlightColorLight.b * pulse);
                    boxColor = { r, g, b, 200 };
                }
                else {
                    boxColor = { 80, 80, 120, 140 };
                }

                SDL_Rect boxRect = { xPosition, startY + int(i) * lineHeight, boxWidth, boxHeight };
                RenderRoundedRect(renderer, boxRect, boxColor, 10);

                stringstream ss;
                ss << store[i].name << "   |   Price: $" << store[i].price << "   |   Quantity: " << store[i].quantity;

                SDL_Texture* textTex = RenderText(renderer, font, ss.str(), baseTextColor);
                if (textTex) {
                    int w, h;
                    SDL_QueryTexture(textTex, nullptr, nullptr, &w, &h);
                    SDL_Rect textRect = { boxRect.x + 15, boxRect.y + 5, w, h };
                    SDL_RenderCopy(renderer, textTex, nullptr, &textRect);
                    SDL_DestroyTexture(textTex);
                }

                SDL_Texture* descTex = RenderText(renderer, font, store[i].description, baseTextColor);
                if (descTex) {
                    int w, h;
                    SDL_QueryTexture(descTex, nullptr, nullptr, &w, &h);
                    SDL_Rect descRect = { boxRect.x + 15, boxRect.y + 5 + 26, w, h };
                    SDL_RenderCopy(renderer, descTex, nullptr, &descRect);
                    SDL_DestroyTexture(descTex);
                }
            }

            stringstream balanceStream;
            balanceStream << "Balance: $" << balance;
            SDL_Texture* balanceTex = RenderText(renderer, font, balanceStream.str(), baseTextColor);
            if (balanceTex) {
                int w, h;
                SDL_QueryTexture(balanceTex, nullptr, nullptr, &w, &h);
                SDL_Rect balanceRect = { 20, 20, w, h };
                SDL_RenderCopy(renderer, balanceTex, nullptr, &balanceRect);
                SDL_DestroyTexture(balanceTex);
            }

            auto DrawButtonWithLabel = [&](SDL_Rect rect, const string& label) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                bool hovered = IsPointInRect(mx, my, rect);
                SDL_Color color = hovered ? SDL_Color{ 255,180,180,220 } : SDL_Color{ 60,60,90,160 };
                RenderRoundedRect(renderer, rect, color, 8);
                SDL_Texture* textTex = RenderText(renderer, font, label, baseTextColor);
                if (textTex) {
                    int w, h;
                    SDL_QueryTexture(textTex, nullptr, nullptr, &w, &h);
                    SDL_Rect textRect = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
                    SDL_RenderCopy(renderer, textTex, nullptr, &textRect);
                    SDL_DestroyTexture(textTex);
                }
                };

            DrawButtonWithLabel(btnUp, "Up");
            DrawButtonWithLabel(btnDown, "Down");
            DrawButtonWithLabel(btnAdd, "Add");
            DrawButtonWithLabel(btnDelete, "Delete");
            DrawButtonWithLabel(btnSell, "Sell");
            DrawButtonWithLabel(btnEdit, "Edit");
            DrawButtonWithLabel(btnBack, "Menu");

            SDL_RenderPresent(renderer);
        }
        else if (state == AppState::EDIT) {
            SDL_SetRenderDrawColor(renderer, 40, 40, 70, 255);
            SDL_RenderClear(renderer);

            int lineHeight = 40;
            int inputFieldHeight = 36;
            int marginTop = winHeight / 5;
            int inputWidth = winWidth - 100;

            SDL_Rect nameLabelRect = { 50, marginTop - 28, 300, 24 };
            SDL_Rect nameInputRect = { 50, marginTop, inputWidth, inputFieldHeight };

            SDL_Rect priceLabelRect = { 50, marginTop + (lineHeight * 2) - 28, 300, 24 };
            SDL_Rect priceInputRect = { 50, marginTop + (lineHeight * 2), inputWidth, inputFieldHeight };

            SDL_Rect descLabelRect = { 50, marginTop + (lineHeight * 4) - 28, 300, 24 };
            SDL_Rect descInputRect = { 50, marginTop + (lineHeight * 4), inputWidth, inputFieldHeight * 3 };

            int btnWidth = 150;
            int btnHeight = 50;
            int btnY = winHeight - 80;
            SDL_Rect btnSave = { winWidth / 2 - btnWidth - 20, btnY, btnWidth, btnHeight };
            SDL_Rect btnCancel = { winWidth / 2 + 20, btnY, btnWidth, btnHeight };

            auto DrawLabel = [&](SDL_Rect rect, const string& text) {
                SDL_Texture* tex = RenderText(renderer, font, text, baseTextColor);
                if (tex) {
                    int w, h;
                    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                    SDL_Rect dst = { rect.x, rect.y, w, h };
                    SDL_RenderCopy(renderer, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                }
                };

            DrawLabel(nameLabelRect, "Toy Name");
            DrawLabel(priceLabelRect, "Price");
            DrawLabel(descLabelRect, "Description:");

            auto DrawInputBox = [&](SDL_Rect rect, bool focused) {
                SDL_Color bgColor = focused ? SDL_Color{ 60, 60, 90, 220 } : SDL_Color{ 40, 40, 70, 180 };
                SDL_Color borderColor = focused ? SDL_Color{ 255, 180, 180, 255 } : SDL_Color{ 80, 80, 120, 255 };
                RenderRoundedRect(renderer, rect, bgColor, 8);
                SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
                SDL_Rect borderRect = { rect.x - 2, rect.y - 2, rect.w + 4, rect.h + 4 };
                SDL_RenderDrawRect(renderer, &borderRect);
                };

            DrawInputBox(nameInputRect, editFocusedField == 0);
            DrawInputBox(priceInputRect, editFocusedField == 1);
            DrawInputBox(descInputRect, editFocusedField == 2);

            auto RenderInputText = [&](SDL_Rect rect, const string& text, bool focused) {
                SDL_Texture* textTex = RenderText(renderer, font, text, baseTextColor);
                if (textTex) {
                    int w, h;
                    SDL_QueryTexture(textTex, nullptr, nullptr, &w, &h);
                    if (w > rect.w - 10) w = rect.w - 10;
                    SDL_Rect dst = { rect.x + 5, rect.y + (rect.h - h) / 2, w, h };
                    SDL_RenderCopy(renderer, textTex, nullptr, &dst);

                    if (focused) {
                        Uint32 ticks = SDL_GetTicks();
                        int cursorX = dst.x + w + 1;
                        int cursorY = dst.y;
                        DrawCursor(renderer, cursorX, cursorY, h, ticks);
                    }

                    SDL_DestroyTexture(textTex);
                }
                };

            RenderInputText(nameInputRect, editName, editFocusedField == 0);
            RenderInputText(priceInputRect, editPriceStr, editFocusedField == 1);
            RenderInputText(descInputRect, editDescription, editFocusedField == 2);

            auto DrawButton = [&](SDL_Rect rect, const string& label) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                bool hovered = IsPointInRect(mx, my, rect);
                SDL_Color color = hovered ? SDL_Color{ 255,180,180,220 } : SDL_Color{ 60,60,90,180 };
                RenderRoundedRect(renderer, rect, color, 12);
                SDL_Texture* tex = RenderText(renderer, font, label, baseTextColor);
                if (tex) {
                    int w, h;
                    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                    SDL_Rect dst = { rect.x + (rect.w - w) / 2, rect.y + (rect.h - h) / 2, w, h };
                    SDL_RenderCopy(renderer, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                }
                };

            DrawButton(btnSave, "Save");
            DrawButton(btnCancel, "Cancel");

            SDL_RenderPresent(renderer);
        }
    }

    SDL_StopTextInput();

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

