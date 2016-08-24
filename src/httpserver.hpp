#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <functional>

#include "json.hpp"

namespace HTTP {

// helpers
std::string iptostr(uint32_t netorderip);
std::string path( // valid iff != ""
  const std::vector<std::string>& segments,
  const std::string& dir_path = ""
);

class Session {
  public:
    virtual ~Session();
    virtual Session* clone() const = 0;
  protected:
    static void destroy(const std::function<bool(const Session*)>&);
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
      const std::function<void(const std::vector<std::string>& args)>&,
      bool session_required = false,
      bool post_required = false,
      int min_args_required = 0
    );
    virtual void not_found();
    virtual void unauthorized(); // for routes with 'session required' flag
    void location(const std::string&); // redirect
    // socket
    time_t when() const;
    uint32_t ip() const; // network order
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
    template <typename T>
    bool header(const std::string& name, T& buf) {
      std::stringstream ss(header(name));
      T ans;
      if (!(ss >> ans)) return false;
      ss.get();
      return !ss;
    }
    std::vector<uint8_t>& payload();
    const std::vector<uint8_t>& payload() const;
    // response
    void status(
      int code,
      const std::string& phrase,
      const std::string& version = "HTTP/1.1"
    );
    void header(const std::string& name, const std::string& value);
    void response(const std::string& data,const std::string& type = "");
    void json(const JSON&);
    void file(const std::string& path, const std::string& type = "");
    void attachment(const std::string& path);
    // session
    Session* session() const;
    void session(Session*);
  // implementation
  private:
    // routing
    struct Route {
      std::function<void(const std::vector<std::string>&)> func;
      bool session;
      bool post;
      int min_args;
    };
    std::map<std::string,Route> routes;
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
  const JSON& settings,
  std::function<bool()> alive,
  std::function<Handler*()> handler_factory = []() { return new Handler; }
);

} // namespace HTTP

#endif
