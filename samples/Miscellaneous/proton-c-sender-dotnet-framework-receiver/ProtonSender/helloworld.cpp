#include <proton/connection.hpp>
#include <proton/connection_options.hpp>
#include <proton/container.hpp>
#include <proton/default_container.hpp>
#include <proton/delivery.hpp>
#include <proton/message.hpp>
#include <proton/messaging_handler.hpp>
#include <proton/thread_safe.hpp>
#include <proton/tracker.hpp>
#include <proton/url.hpp>

#include <iostream>

#include "fake_cpp11.hpp"

class hello_world : public proton::messaging_handler {
  private:
    proton::url url;
    int send_count;
    int accept_count;

  public:
    hello_world(const proton::url& u) : url(u) {
        send_count = 0;
        accept_count = 0;
    }

    void on_container_start(proton::container& c) OVERRIDE {
        c.connect(url);
    }

    void on_connection_open(proton::connection& c) OVERRIDE {
        c.open_sender(url.path());
    }

    void on_sendable(proton::sender &s) OVERRIDE {
        if (send_count++ < 200) {
            proton::message m(proton::binary("Hello World from C++!"));
            m.inferred(true);
            s.send(m);
        }
    }

    void on_tracker_accept(proton::tracker &t) OVERRIDE {
        if (++accept_count == 200) {
            t.connection().close();
        }
    }
};

int main(int argc, char **argv) {
    try {
        if (argc <= 1) {
           std::cout << "helloworld url" << std::endl;
           return 2;
        }

        proton::url url = proton::url(argv[1]);
        hello_world hw(url.scheme() + "://" + url.host() + ":" + url.port() + "/" + url.path());
        proton::default_container container(hw);
        container.client_connection_options(proton::connection_options().sasl_enabled(true).user(url.user()).password(url.password()));
        container.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 1;
}

