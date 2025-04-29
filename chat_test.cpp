#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib> // 用于getenv

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
namespace asio = websocketpp::lib::asio;

// 定义WebSocket客户端类型
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

class ChatTester {
public:
    ChatTester() {
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
        c.set_tls_init_handler(bind(&ChatTester::on_tls_init, this, ::_1));
        
        // 设置消息处理回调
        c.set_message_handler(bind(&ChatTester::on_message, this, ::_1, ::_2));
        c.set_open_handler(bind(&ChatTester::on_open, this, ::_1));
        c.set_fail_handler(bind(&ChatTester::on_fail, this, ::_1));
        c.set_close_handler(bind(&ChatTester::on_close, this, ::_1));
    }
    
    void run_test() {
        websocketpp::lib::error_code ec;
        
        // 创建连接
        client::connection_ptr con = c.get_connection(chat_ws_v2_endpoint, ec);
        if (ec) {
            std::cout << "连接创建失败: " << ec.message() << std::endl;
            return;
        }
        
        // 添加自定义header
        con->append_header("X-API-KEY", api_key);
        
        // 建立连接
        c.connect(con);
        
        // 运行事件循环
        c.run();
    }
    
private:
    context_ptr on_tls_init(websocketpp::connection_hdl) {
        context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);
        try {
            ctx->set_options(asio::ssl::context::default_workarounds |
                           asio::ssl::context::no_sslv2 |
                           asio::ssl::context::no_sslv3 |
                           asio::ssl::context::single_dh_use);
        } catch (std::exception& e) {
            std::cout << "Error in TLS init: " << e.what() << std::endl;
        }
        return ctx;
    }
    
    void on_open(websocketpp::connection_hdl hdl) {
        std::cout << "连接已打开" << std::endl;
        
        // 读取并发送音频文件
        std::string audio_file = "cases/case_6_xiaoyanxiaoyan.pcm";
        std::ifstream file(audio_file, std::ios::binary);
        if (!file) {
            std::cout << "无法打开音频文件: " << audio_file << std::endl;
            return;
        }
        
        // 读取文件内容
        std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
        file.close();
        
        // 发送二进制数据
        try {
            c.send(hdl, buffer.data(), buffer.size(), websocketpp::frame::opcode::binary);
            std::cout << "已发送音频数据，大小: " << buffer.size() << " bytes" << std::endl;
        } catch (const websocketpp::exception& e) {
            std::cout << "发送失败: " << e.what() << std::endl;
        }
    }
    
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
        if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
            // 处理二进制音频数据
            std::cout << "收到音频数据" << std::endl;
            
            // 创建output目录
            std::filesystem::create_directory("output");
            
            // 保存音频数据
            std::string filename = "output/audio_response_" + 
                                 std::to_string(std::time(nullptr)) + ".wav";
            std::ofstream outfile(filename, std::ios::binary);
            if (outfile) {
                outfile.write(msg->get_payload().c_str(), msg->get_payload().size());
                std::cout << "已保存音频数据到 " << filename << std::endl;
            }
        } else {
            // 处理JSON响应
            std::cout << "收到文本回复: " << msg->get_payload() << std::endl;
        }
    }
    
    void on_fail(websocketpp::connection_hdl hdl) {
        std::cout << "连接失败" << std::endl;
    }
    
    void on_close(websocketpp::connection_hdl hdl) {
        std::cout << "连接已关闭" << std::endl;
    }
    
    client c;
    std::string base_url;
    std::string chat_ws_v2_endpoint;
    std::string api_key;
};

int main() {
    ChatTester tester;
    tester.run_test();
    return 0;
} 