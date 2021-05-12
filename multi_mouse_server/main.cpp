#include <iostream>
#include <vector>
#include <string>
#include "include/easysocket.hpp"
#include <WinUser.h>
#include <windows.h>
#include <map>

#ifndef UNICODE
#define UNICODE
#define UNICODE_WAS_UNDEFINED
#endif

using namespace std;

const float max_res = 65536.0f;
uint8_t num_clients = 0;
const uint8_t max_clients = 10;
const int size = 16;

struct Client {
    std::string id;
    std::string num;
    COLORREF color;
    RECT last_position;
};

std::map<std::string, Client> clients;

int res_x = 1920;
int res_y = 1080;
RECT client_rect;

HWND d_hwnd;
HWND hwnd;
HDC hdc;

bool click_and_return(float x, float y) {
    if (x > max_res || y > max_res)
        return false;
    if (x < 0 || y < 0)
        return false;
    HWND hwnd = GetDesktopWindow(); 
    
    POINT previous_point;
    GetCursorPos(&previous_point);

    INPUT click_instructions[3] = {0};
    ZeroMemory(click_instructions, sizeof(click_instructions));

    click_instructions[0].type = INPUT_MOUSE;
    click_instructions[0].mi.dx = static_cast<int>(x * max_res);
    click_instructions[0].mi.dy = static_cast<int>(y * max_res);
    click_instructions[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    click_instructions[1].type = INPUT_MOUSE;
    click_instructions[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    click_instructions[2].type = INPUT_MOUSE;
    click_instructions[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    UINT8 num_input = 3;
    SendInput(num_input, click_instructions, sizeof(INPUT));

    INPUT position_return_instructions[1] = {0};
    ZeroMemory(position_return_instructions, sizeof(position_return_instructions));

    float p_x = static_cast<float>(previous_point.x);
    float p_y = static_cast<float>(previous_point.y);
    float width = static_cast<float>(client_rect.right) * 65536.0f;
    float height = static_cast<float>(client_rect.bottom) * 65536.0f;

    position_return_instructions[0].type = INPUT_MOUSE;
    position_return_instructions[0].mi.dx = static_cast<int>(p_x / width);
    position_return_instructions[0].mi.dy = static_cast<int>(p_y / height);
    position_return_instructions[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    SendInput(1, position_return_instructions, sizeof(INPUT));
    return true;
}

bool draw_cursor(float x, float y, int size, std::string client_id) {
    auto client = clients.at(client_id);
    auto brush = CreateSolidBrush(client.color);
    auto rect = new RECT;
    const RECT* client_rect = &client.last_position;
    x = static_cast<int>(x * res_x);
    y = static_cast<int>(y * res_y);
    rect->left = x;
    rect->right = x + size;
    rect->top = y;
    rect->bottom = y + size;
    FillRect(hdc, rect, brush);
    InvalidateRect(hwnd, client_rect, FALSE);
    return true;
}

int create_new_client(std::string id) {
    uint8_t client_num = num_clients;
    num_clients++;
    Client client;
    client.id = id;
    client.num = client_num;
    client.color = RGB(255, 128, 0);
    RECT last_position;
    last_position.left = 0;
    last_position.right = 16;
    last_position.top = 0;
    last_position.bottom = 16;
    client.last_position = last_position;
    clients.insert(std::pair<std::string, Client>(id, client));
    return client_num;
}

std::pair<int, float> parse_float(std::string fl, int start) {
    for (int i = start; i < fl.length(); i++) {
        if (fl.at(i) == '|') 
            return std::pair<int, float>(i, std::stof(fl.substr(start + 1, i - start)));
    }
}

std::pair<float, float> get_coords(std::string data) {
    auto result = parse_float(data, 3);
    auto cursor = result.first + 1;
    auto pos_x = result.second;
    auto pos_y = std::stof(data.substr(cursor, data.length() - cursor));
    return std::pair<float, float>(pos_x, pos_y);
}

void dispatch(const std::string &data) {
	auto client = data.substr(0, 3);
    if (data.at(3) == 'h')
        create_new_client(client);
    else if (data.at(3) == 'm') {
        auto result = get_coords(data);
        draw_cursor(result.first, result.second, 16, client);
    }
    else if (data.at(3) == 'c') {
        auto result = get_coords(data);
        click_and_return(result.first, result.second);
    }
    else {
        cout << "fuck off";
    }
}

int main() {
    d_hwnd = GetDesktopWindow();

    GetClientRect(d_hwnd, &client_rect);

    hwnd = CreateWindowExA(
        WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, 
        "STATIC", 
        NULL, 
        WS_POPUP | WS_DISABLED,
        0,
        0,
        client_rect.right,
        client_rect.bottom,
        d_hwnd,
        NULL,
        NULL,
        NULL
    );

    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_COLORKEY);

    hdc = GetDC(hwnd);

    ShowWindow(hwnd, 1);

    masesk::EasySocket socketManager;
	socketManager.socketListen("remote_cursor", 3333, &dispatch);
    ReleaseDC(hwnd, hdc);
    return 0;
}
