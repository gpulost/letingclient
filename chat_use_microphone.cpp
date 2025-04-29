#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <json/json.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <portaudio.h>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
namespace asio = websocketpp::lib::asio;

// 定义WebSocket客户端类型
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

// 音频参数
#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 1024
#define NUM_CHANNELS 1
#define PA_SAMPLE_TYPE paFloat32
typedef float SAMPLE;

class MicrophoneChatClient {
public:
    MicrophoneChatClient() {
        // 初始化WebSocket客户端和相关参数
        base_url = "wss://api.fusearch.cn";
        chat_ws_v2_endpoint = base_url + "/ai/chat_ws_v2";
        
        // 从环境变量获取API密钥
        const char* env_api_key = std::getenv("API_KEY");
        if (env_api_key) {
            api_key = env_api_key;
        } else {
            std::cerr << "错误: 未设置API_KEY环境变量" << std::endl;
            exit(1);
        }
        
        // 初始化WebSocket客户端
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);
        c.set_error_channels(websocketpp::log::elevel::all);
        
        c.init_asio();
        
        // 设置TLS处理
        c.set_tls_init_handler(bind(&MicrophoneChatClient::on_tls_init, this, ::_1));
        
        // 设置消息处理回调
        c.set_message_handler(bind(&MicrophoneChatClient::on_message, this, ::_1, ::_2));
        c.set_open_handler(bind(&MicrophoneChatClient::on_open, this, ::_1));
        c.set_fail_handler(bind(&MicrophoneChatClient::on_fail, this, ::_1));
        c.set_close_handler(bind(&MicrophoneChatClient::on_close, this, ::_1));
        
        // 初始化PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cerr << "初始化PortAudio失败: " << Pa_GetErrorText(err) << std::endl;
            exit(1);
        }
        
        is_recording = false;
        is_connected = false;
    }
    
    ~MicrophoneChatClient() {
        // 确保停止录音
        stop_recording();
        
        // 终止PortAudio
        Pa_Terminate();
    }
    
    // 开始运行WebSocket客户端
    void run() {
        websocketpp::lib::error_code ec;
        
        // 创建连接
        client::connection_ptr con = c.get_connection(chat_ws_v2_endpoint, ec);
        if (ec) {
            std::cout << "连接创建失败: " << ec.message() << std::endl;
            return;
        }
        
        // 添加自定义header
        con->append_header("X-API-KEY", api_key);
        
        // 存储连接
        hdl = con->get_handle();
        
        // 建立连接
        c.connect(con);
        
        // 在另一个线程中运行事件循环，以免阻塞主线程
        std::thread ws_thread([this]() {
            this->c.run();
        });
        
        // 等待用户操作
        std::cout << "按 's' 开始录音，按 'e' 结束录音，按 'q' 退出程序" << std::endl;
        char input;
        while (std::cin >> input) {
            if (input == 's') {
                start_recording();
            } else if (input == 'e') {
                stop_recording();
            } else if (input == 'q') {
                // 停止录音并关闭连接
                stop_recording();
                
                // 关闭WebSocket连接
                websocketpp::lib::error_code ec;
                c.close(hdl, websocketpp::close::status::normal, "用户退出", ec);
                if (ec) {
                    std::cout << "关闭连接时出错: " << ec.message() << std::endl;
                }
                
                break;
            }
        }
        
        // 等待WebSocket线程结束
        if (ws_thread.joinable()) {
            ws_thread.join();
        }
    }
    
private:
    // TLS初始化
    context_ptr on_tls_init(websocketpp::connection_hdl) {
        context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);
        try {
            ctx->set_options(asio::ssl::context::default_workarounds |
                          asio::ssl::context::no_sslv2 |
                          asio::ssl::context::no_sslv3 |
                          asio::ssl::context::single_dh_use);
        } catch (std::exception& e) {
            std::cout << "TLS初始化错误: " << e.what() << std::endl;
        }
        return ctx;
    }
    
    // WebSocket连接建立时的回调
    void on_open(websocketpp::connection_hdl hdl) {
        std::cout << "WebSocket连接已打开" << std::endl;
        is_connected = true;
    }
    
    // 接收消息的回调
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
        if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
            // 处理二进制音频数据
            std::cout << "收到音频数据，大小: " << msg->get_payload().size() << " bytes" << std::endl;
            
            // 这里可以添加音频播放逻辑，或保存到文件
        } else {
            // 处理JSON响应
            std::cout << "收到文本回复: " << msg->get_payload() << std::endl;
        }
    }
    
    // 连接失败的回调
    void on_fail(websocketpp::connection_hdl hdl) {
        std::cout << "WebSocket连接失败" << std::endl;
        is_connected = false;
    }
    
    // 连接关闭的回调
    void on_close(websocketpp::connection_hdl hdl) {
        std::cout << "WebSocket连接已关闭" << std::endl;
        is_connected = false;
    }
    
    // PortAudio流回调函数
    static int pa_callback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {
        MicrophoneChatClient *client = (MicrophoneChatClient *)userData;
        return client->process_audio(inputBuffer, outputBuffer, framesPerBuffer);
    }
    
    // 处理音频数据
    int process_audio(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer) {
        const SAMPLE *in = (const SAMPLE*)inputBuffer;
        
        if (!in) {
            // 没有输入数据，可能是设备断开
            return paContinue;
        }
        
        // 将音频数据复制到缓冲区
        std::vector<char> buffer(framesPerBuffer * NUM_CHANNELS * sizeof(SAMPLE));
        memcpy(buffer.data(), in, buffer.size());
        
        // 发送音频数据到WebSocket
        if (is_connected) {
            try {
                websocketpp::lib::error_code ec;
                c.send(hdl, buffer.data(), buffer.size(), websocketpp::frame::opcode::binary, ec);
                if (ec) {
                    std::cout << "发送音频数据失败: " << ec.message() << std::endl;
                }
            } catch (const websocketpp::exception& e) {
                std::cout << "发送失败: " << e.what() << std::endl;
            }
        }
        
        return paContinue;
    }
    
    // 开始录音
    void start_recording() {
        if (is_recording) {
            std::cout << "已经在录音中" << std::endl;
            return;
        }
        
        if (!is_connected) {
            std::cout << "WebSocket未连接，无法发送音频数据" << std::endl;
            return;
        }
        
        PaError err;
        
        // 设置输入参数
        PaStreamParameters inputParameters;
        inputParameters.device = Pa_GetDefaultInputDevice();
        if (inputParameters.device == paNoDevice) {
            std::cerr << "未找到默认输入设备" << std::endl;
            return;
        }
        
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
        std::cout << "使用麦克风: " << deviceInfo->name << std::endl;
        
        inputParameters.channelCount = NUM_CHANNELS;
        inputParameters.sampleFormat = PA_SAMPLE_TYPE;
        inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;
        
        // 打开音频流
        err = Pa_OpenStream(&stream,
                           &inputParameters,
                           nullptr,  // 没有输出
                           SAMPLE_RATE,
                           FRAMES_PER_BUFFER,
                           paClipOff,
                           pa_callback,
                           this);
        
        if (err != paNoError) {
            std::cerr << "打开音频流失败: " << Pa_GetErrorText(err) << std::endl;
            return;
        }
        
        // 启动音频流
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cerr << "启动音频流失败: " << Pa_GetErrorText(err) << std::endl;
            Pa_CloseStream(stream);
            return;
        }
        
        is_recording = true;
        std::cout << "开始录音..." << std::endl;
    }
    
    // 停止录音
    void stop_recording() {
        if (!is_recording) {
            return;
        }
        
        PaError err = Pa_StopStream(stream);
        if (err != paNoError) {
            std::cerr << "停止音频流失败: " << Pa_GetErrorText(err) << std::endl;
        }
        
        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            std::cerr << "关闭音频流失败: " << Pa_GetErrorText(err) << std::endl;
        }
        
        is_recording = false;
        std::cout << "停止录音" << std::endl;
    }
    
    client c;
    websocketpp::connection_hdl hdl;
    std::string base_url;
    std::string chat_ws_v2_endpoint;
    std::string api_key;
    
    PaStream *stream;
    bool is_recording;
    bool is_connected;
};

int main() {
    MicrophoneChatClient client;
    client.run();
    return 0;
}
