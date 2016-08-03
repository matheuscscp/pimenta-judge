#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <functional>
#include <iostream>

#include "json.hpp"

namespace HTTP {

class Session {
  public:
    virtual ~Session();
    virtual Session* clone() const = 0;
};

class Handler {
  // API
  public:
    Handler(); // only route() method works properly inside constructors
    virtual ~Handler();
  protected:
    // routing
    void route(
      const std::string& path,
      const std::function<void(const std::vector<std::string>& args)>&
    );
    virtual void notfound();
    // socket
    time_t when() const;
    uint32_t ip() const; // network endianness
    // request
    std::string& method();
    const std::string& method() const;
    std::string& uri();
    const std::string& uri() const;
    std::string& version();
    const std::string& version() const;
    std::string& path();
    const std::string& path() const;
    std::string& query();
    const std::string& query() const;
    std::string header(const std::string& name) const;
    std::vector<uint8_t>& payload();
    const std::vector<uint8_t>& payload() const;
    // response
    void status(
      int code,
      const std::string& phrase,
      const std::string& version = "HTTP/1.1"
    );
    void header(const std::string& name, const std::string& value);
    void response(const std::string& data, const std::string& type);
    void response(const JSON&);
    void file(const std::string& path, const std::string& type = "");
    // session
    Session* session() const;
    void session(Session*);
  // implementation
  private:
    // routing
    std::map<
      std::string,
      std::function<void(const std::vector<std::string>&)>
    > routes;
    // socket
    int sd;
    time_t when_;
    uint32_t ip_;
    // request
    std::string buf, method_, uri_, version_, path_, query_;
    std::map<std::string,std::string> req_headers;
    std::vector<uint8_t> payload_;
    // response
    int st_code;
    std::string st_phrase, st_version;
    std::map<std::string,std::string> resp_headers;
    std::string data; bool isfile;
    // session
    Session* sess;
  public:
    void init(int,time_t,uint32_t);
    void handle();
  private:
    bool getline(int);
};

void server(
  std::function<bool()> alive,
  const JSON& settings = JSON(),
  std::function<Handler*()> handler_factory = []() { return new Handler; },
  std::ostream& log = std::clog
);

} // namespace HTTP

#endif
