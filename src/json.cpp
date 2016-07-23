#include <map>
#include <stack>
#include <sstream>
#include <algorithm>

#include "json.h"

using namespace std;

static JSON* JSONerror = nullptr;

struct JSONValue {
  virtual ~JSONValue() {}
  virtual JSONValue* clone() const = 0;
  virtual string str(int) const = 0;
  virtual bool isstr() const = 0;
  virtual JSON& operator[](const string&) = 0;
  virtual const JSON* find(const string&) const = 0;
  virtual vector<string> keys() const = 0;
  virtual JSON& operator[](size_t i) = 0;
  virtual bool isarr() const = 0;
  virtual void push_back(const JSON&) = 0;
  virtual size_t erase(size_t, size_t) = 0;
  virtual size_t size() const = 0;
};

struct String : public JSONValue {
  string v;
  String(const string& v) : v(v) {}
  JSONValue* clone() const {
    return new String(v);
  }
  string str(int) const {
    return v;
  }
  bool isstr() const {
    return true;
  }
  JSON& operator[](const string&) {
    return *JSONerror;
  }
  const JSON* find(const string&) const {
    return nullptr;
  }
  vector<string> keys() const {
    return vector<string>();
  }
  JSON& operator[](size_t) {
    return *JSONerror;
  }
  bool isarr() const {
    return false;
  }
  void push_back(const JSON&) {
    
  }
  size_t erase(size_t, size_t) {
    
  }
  size_t size() const {
    return v.size();
  }
};

struct Object : public JSONValue {
  map<string,JSON> v;
  Object(const map<string,JSON>& v = map<string,JSON>()) : v(v) {}
  JSONValue* clone() const {
    return new Object(v);
  }
  string str(int indent) const {
    string ans = "{"; if (indent > 0) ans += "\n";
    auto cnt = v.size();
    for (auto& kv : v) {
      for (int i = 0; i < indent; i++) ans += "  ";
      ans += JSON::str(kv.first);
      ans += " : ";
      if (!kv.second.isstr()) ans += kv.second.str(indent ? indent+1 : 0);
      else ans += JSON::str(kv.second.str(indent ? indent+1 : 0));
      cnt--;
      if (cnt > 0) ans += ",";
      if (indent > 0) ans += "\n";
    }
    for (int i = 0; i < indent-1; i++) ans += "  ";
    ans += "}";
    return ans;
  }
  bool isstr() const {
    return false;
  }
  JSON& operator[](const string& key) {
    return v[key];
  }
  const JSON* find(const string& key) const {
    auto it = v.find(key);
    return (it == v.end() ? nullptr : &it->second);
  }
  vector<string> keys() const {
    vector<string> ans(v.size());
    size_t i = 0; for (auto& kv : v) ans[i++] = kv.first;
    return ans;
  }
  JSON& operator[](size_t) {
    return *JSONerror;
  }
  bool isarr() const {
    return false;
  }
  void push_back(const JSON&) {
    
  }
  size_t erase(size_t, size_t) {
    
  }
  size_t size() const {
    return v.size();
  }
};

struct Array : public JSONValue {
  vector<JSON> v;
  Array(const vector<JSON>& v = vector<JSON>()) : v(v) {}
  JSONValue* clone() const {
    return new Array(v);
  }
  string str(int indent) const {
    string ans = "["; if (indent > 0) ans += "\n";
    auto cnt = v.size();
    for (auto& obj : v) {
      for (int i = 0; i < indent; i++) ans += "  ";
      if (!obj.isstr()) ans += obj.str(indent ? indent+1 : 0);
      else ans += JSON::str(obj.str(indent ? indent+1 : 0));
      cnt--;
      if (cnt > 0) ans += ",";
      if (indent > 0) ans += "\n";
    }
    for (int i = 0; i < indent-1; i++) ans += "  ";
    ans += "]";
    return ans;
  }
  bool isstr() const {
    return false;
  }
  JSON& operator[](const string&) {
    return *JSONerror;
  }
  const JSON* find(const string&) const {
    return nullptr;
  }
  vector<string> keys() const {
    return vector<string>();
  }
  JSON& operator[](size_t i) {
    return v[i];
  }
  bool isarr() const {
    return true;
  }
  void push_back(const JSON& val) {
    v.push_back(val);
  }
  size_t erase(size_t first, size_t last) {
    auto it = v.erase(v.begin()+first,v.begin()+last);
    return it-v.begin();
  }
  size_t size() const {
    return v.size();
  }
};

JSON::JSON() : value(new Object) {
  
}

JSON::~JSON() {
  delete value;
}

JSON::JSON(const JSON& val) : value(nullptr) {
  operator=(val);
}

JSON& JSON::operator=(const JSON& val) {
  delete value;
  value = (val.value ? val.value->clone() : nullptr);
  return *this;
}

JSON::JSON(JSON&& val) : value(nullptr) {
  operator=(move(val));
}

JSON& JSON::operator=(JSON&& val) {
  swap(value,val.value);
  return *this;
}

JSON::operator string() const {
  return str();
}

string JSON::str(int indent) const {
  return value->str(indent);
}

JSON::JSON(const char* val) : value(nullptr) {
  operator=(string(val));
}

JSON& JSON::operator=(const char* val) {
  return operator=(string(val));
}

JSON::JSON(const string& val) : value(nullptr) {
  operator=(val);
}

JSON& JSON::operator=(const string& val) {
  delete value;
  value = new String(val);
  return *this;
}

bool JSON::operator==(const string& val) const {
  return true;
}

bool JSON::isstr() const {
  return value->isstr();
}

JSON& JSON::operator[](const string& key) {
  return (*value)[key];
}

const JSON* JSON::find(const string& key) const {
  return value->find(key);
}

vector<string> JSON::keys() const {
  return value->keys();
}

JSON& JSON::operator[](size_t i) {
  return (*value)[i];
}

const JSON& JSON::operator[](size_t i) const {
  return (*value)[i];
}

JSON::JSON(const initializer_list<JSON>& val) : value(nullptr) {
  operator=(vector<JSON>(val));
}

JSON& JSON::operator=(const initializer_list<JSON>& val) {
  return operator=(vector<JSON>(val));
}

JSON::JSON(const vector<JSON>& val) : value(nullptr) {
  operator=(val);
}

JSON& JSON::operator=(const vector<JSON>& val) {
  delete value;
  value = new Array(val);
  return *this;
}

bool JSON::isarr() const {
  return value->isarr();
}

void JSON::push_back(const JSON& val) {
  value->push_back(val);
}

size_t JSON::erase(size_t first, size_t last) {
  value->erase(first,last);
}

size_t JSON::size() const {
  return value->size();
}

JSON& JSON::operator()() {
  return *this;
}

const JSON* JSON::find() const {
  return this;
}

static char tohex(char c) {
  if (c < 10) return c+'0';
  return c-10+'a';
}
string JSON::str(const string& val) {
  if (
    number_size(val) == val.size() ||
    val == "true" || val == "false" || val == "null"
  ) return val;
  string ans = "\"";
  for (char c : val) switch (c) {
    case '"':   ans += "\\\"";  break;
    case '\\':  ans += "\\\\";  break;
    case '/':   ans += "\\/";   break;
    case '\b':  ans += "\\b";   break;
    case '\f':  ans += "\\f";   break;
    case '\n':  ans += "\\n";   break;
    case '\r':  ans += "\\r";   break;
    case '\t':  ans += "\\t";   break;
    default: {
      int ch = c&0x0ff;
      if (
        // unicode C0 control characters
        (0x0000 <= ch && ch <= 0x001f) || ch == 0x007f ||
        // unicode C1 control characters
        (0x0080 <= ch && ch <= 0x009f)
      ) {
        ans += "\\u00";
        ans += tohex((c>>4)&0x0f);
        ans += tohex(c&0x0f);
      }
      else ans += c;
      break;
    }
  }
  return ans+"\"";
}

size_t JSON::number_size(const string& val) {
  const char* s = val.c_str();
  if (*s == '-') s++;
  if (*s < '0' || '9' < *s) return 0;
  if (*s == '0') s++;
  else while ('0' <= *s && *s <= '9') s++;
  if (*s == '.') {
    s++;
    if (*s < '0' || '9' < *s) return 0;
    while ('0' <= *s && *s <= '9') s++;
  }
  size_t backup = s-val.c_str();
  if (*s != 'e' && *s != 'E') return backup;
  s++;
  if (*s == '+' || *s == '-') s++;
  else if (*s < '0' || '9' < *s) return backup;
  while ('0' <= *s && *s <= '9') s++;
  return s-val.c_str();
}

bool operator==(const string& val, const JSON& json) {
  return json == val;
}

ostream& operator<<(ostream& os, const JSON& json) {
  return os << json.str(1);
}

// =============================================================================
// JSON parser
// =============================================================================

static string unicode(const char* s, string& error) {
  char buf[5] = {};
  for (int i = 0; i < 4 && error == ""; i++) {
    if (
      ('0' <= s[i] && s[i] <= '9') ||
      ('a' <= s[i] && s[i] <= 'f') ||
      ('A' <= s[i] && s[i] <= 'F')
    ) { buf[i] = s[i]; continue; }
    error = "lexical error: expected four hexadecimal digits after '\\u'";
  }
  if (error != "") return "";
  int x;
  sscanf(&buf[2],"%x",&x);
  if (!x) return "";
  string ans;
  ans += char(x);
  buf[2] = 0;
  sscanf(buf,"%x",&x);
  if (!x) return ans;
  ans += char(x);
  return ans;
}

static string parse_string(const string& src, string& error) {
  string ans;
  for (auto s = src.c_str(); *s != 0 && error == ""; s++) {
    if (*s != '\\') { ans += *s; continue; }
    s++;
    switch (*s) {
      case '"':   ans += '"';   break;
      case '\\':  ans += '\\';  break;
      case '/':   ans += '/';   break;
      case 'b':   ans += '\b';  break;
      case 'f':   ans += '\f';  break;
      case 'n':   ans += '\n';  break;
      case 'r':   ans += '\r';  break;
      case 't':   ans += '\t';  break;
      case 'u':
        ans += unicode(s+1,error);
        s += 4;
        break;
      default:
        error = "lexical error: expected escaped character after '\\'";
    }
  }
  if (error != "") return "";
  return ans;
}

enum {STRING=0,NUMBER,CONST,ERROR};
struct Token {
  char type;
  string data;
  Token(char type, const string& data = "") : type(type), data(data) {}
  Token(const string& str) : type(STRING), data(str) {}
};

static void small_tkns(const string& big, vector<Token>& ans, string& err) {
  auto error = [&](int i) {
    err = "lexical error: invalid token '"+big.substr(i,big.size())+"'";
  };
  auto check_const = [&](int& i, const string& constant) {
    string tmp = big.substr(i,constant.size());
    if (tmp != constant) error(i);
    else { ans.emplace_back(CONST,tmp); i += constant.size()-1; }
  };
  string tmp;
  for (int i = 0; i < big.size() && err == ""; i++) {
    switch (big[i]) {
      case '{':
      case '}':
      case ',':
      case ':':
      case '[':
      case ']':
        ans.emplace_back(big[i]);
        break;
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': {
        auto sz = JSON::number_size(big.substr(i,big.size()));
        if (!sz) error(i);
        else {
          ans.emplace_back(NUMBER,big.substr(i,sz));
          i += sz-1;
        }
        break;
      }
      case 't': check_const(i,"true");  break;
      case 'f': check_const(i,"false"); break;
      case 'n': check_const(i,"null");  break;
      default: error(i);
    }
  }
  if (err != "") ans.clear();
}

static vector<Token> tokens(const string& ln, string& error) {
  vector<Token> ans;
  for (int i = 0; i < ln.size();) {
    stringstream ss;
    while (i < ln.size() && ln[i] != '"') ss << ln[i++];
    string big;
    while ((ss >> big) && error == "") small_tkns(big,ans,error);
    if (error != "") return vector<Token>();
    if (i == ln.size()) return ans;
    string s; i++;
    while (i < ln.size() && (ln[i-1] == '\\' || ln[i] != '"')) s += ln[i++];
    if (i == ln.size()) {
      error = "lexical error: expected '\"' before end of line";
      return vector<Token>();
    }
    i++;
    s = parse_string(s,error);
    if (error != "") return vector<Token>();
    ans.emplace_back(s);
  }
  return ans;
}

enum {OBJECT=1,KEY,COLON,VALUE,MEMBERS,ARRAY,ELEMENTS};
struct PDA {
  int state;
  stack<JSON> S;
  JSON json;
  PDA() : state(VALUE) {}
  void push_obj() {
    state = OBJECT;
    S.emplace();
  }
  void push_key(const string& key) {
    state = COLON;
    S.push(key);
  }
  void push_arr() {
    state = ARRAY;
    S.push(JSON({}));
  }
  void pop() {
    json = S.top(); S.pop();
    if (S.size() == 0) state = 0;
    else value(json);
  }
  void value(const JSON& obj) {
    if (S.size() == 0) {
      state = 0;
      json = obj;
    }
    else if (S.top().isstr()) {
      state = MEMBERS;
      string key = S.top(); S.pop();
      S.top()[key] = obj;
    }
    else {
      state = ELEMENTS;
      S.top().push_back(obj);
    }
  }
  void process(const Token& token, string& error) {
    switch (state) {
      case OBJECT:
        if (token.type == '}') pop();
        else if (token.type == STRING) push_key(token.data);
        else error = "syntax error: expected '}' or key";
        break;
      case KEY:
        if (token.type == STRING) push_key(token.data);
        else error = "syntax error: expected key";
        break;
      case COLON:
        if (token.type == ':') state = VALUE;
        else error = "syntax error: expected ':'";
        break;
      case VALUE:
L_VAL:  if (token.type == '{') push_obj();
        else if (token.type == '[') push_arr();
        else if (token.type < ERROR) value(token.data);
        else error = "syntax error: expected value";
        break;
      case MEMBERS:
        if (token.type == '}') pop();
        else if (token.type == ',') state = KEY;
        else error = "syntax error: expected '}' or ','";
        break;
      case ARRAY:
        if (token.type == ']') pop();
        else goto L_VAL;
        break;
      case ELEMENTS:
        if (token.type == ']') pop();
        else if (token.type == ',') state = VALUE;
        else error = "syntax error: expected ']' or ','";
        break;
    }
  }
};

istream& operator>>(istream& is, JSON& json) {
  int ln;
  auto error = [&](const string& msg) -> istream& {
    stringstream ss;
    ss << ln << ": " << msg;
    json = JSON(ss.str());
    return is;
  };
  PDA pda;
  string err;
  for (ln = 1; pda.state && err == ""; ln++) {
    string line;
    if (!getline(is,line)) { pda.process(Token(ERROR),err); break; }
    vector<Token> token = tokens(line,err);
    if (err != "") return error(err);
    for (int i = 0; i < token.size() && pda.state && err == ""; i++) {
      pda.process(token[i],err);
      if (err != "") return error(err);
    }
  }
  if (err != "") return error(err);
  json = pda.json;
  return is;
}
