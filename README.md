# onexplayer-wifigamecontroller

## 简介

本程序使用 C++ 编写，使用pc掌机通过 WiFi 进行图像帧的发送和接收。程序包括了使用 TCP 和 UDP 套接字进行网络通信的功能，以及使用 OpenCV 进行图像处理。该程序还包含了通过 SDL 库进行游戏控制器的支持。

## 依赖项

在编译和运行本程序之前，请确保您的系统安装了以下库：

- OpenCV
- SDL2
- pthread

您可以通过以下命令在 Ubuntu 上安装这些库：

```bash
sudo apt-get update
sudo apt-get install libopencv-dev libsdl2-dev libpthread-stubs0-dev
```

## 编译

在编译程序之前，请确保您已经安装了所有必需的库。然后，使用以下命令编译程序：

```bash
g++ -o wificontrol wificontrol.cpp -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc -lpthread -lSDL2
```

## 运行

编译完成后，可以使用以下命令运行程序：

```bash
./wificontrol
```

## 代码结构

- `send_frames()`: 发送图像帧的函数。
- `receive_frames()`: 接收图像帧的函数。
- `udp_communication()`: 通过 UDP 进行通信的函数。

## 注意事项

- 请确保您的网络环境配置正确，代码中定义的 IP 地址和端口号应与实际情况相符。
- 本程序假设使用 RGB 格式的图像帧，并定义了默认的帧宽和帧高。
- 在绑定和监听套接字时，如果出现错误，程序会等待一段时间后重试。

## 示例代码片段

以下是一个接收图像帧的示例代码片段：

```cpp
void receive_frames() {
    while (true) {
        // 创建 TCP 套接字
        int listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sockfd < 0) {
            std::cerr << "创建套接字时出错" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待后重试
            continue;
        }

        sockaddr_in listen_addr;
        memset(&listen_addr, 0, sizeof(listen_addr));
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_addr.s_addr = INADDR_ANY;
        listen_addr.sin_port = htons(DEST_PORT);

        // 绑定套接字
        if (bind(listen_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
            std::cerr << "绑定套接字时出错" << std::endl;
            close(listen_sockfd);
            std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待后重试
            continue;
        }

        // 监听传入连接
        if (listen(listen_sockfd, 1) < 0) {
            std::cerr << "监听套接字时出错" << std::endl;
            close(listen_sockfd);
            std::this_thread::sleep_for(std::chrono::seconds(5)); // 等待后重试
            continue;
        }

        // ... 省略其余代码 ...
    }
}
```

## 联系方式

如果您在使用过程中遇到任何问题或有任何疑问，请联系作者：2052046346@qq.com


