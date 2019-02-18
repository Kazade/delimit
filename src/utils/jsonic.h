#pragma once

#define JSONIC_USE_STL 1

#ifdef JSONIC_USE_STL
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <cstdlib>
#else
#include "./containers.h"
#endif

namespace jsonic {

class Node;

#ifdef JSONIC_USE_STL
template<typename T>
using Vector = std::vector<T>;

typedef std::string String;
typedef std::map<String, Node*> MapType;
typedef Vector<Node*> ListType;
#else
template<typename T>
using Vector = containers::Vector<T>;

typedef Vector<Node*> ListType;
typedef containers::String String;
typedef containers::HashMap<String, Node*> MapType;
#endif


enum NodeType {
    NODE_TYPE_VALUE = 0,
    NODE_TYPE_LIST,
    NODE_TYPE_DICT
};

enum ValueType {
    VALUE_TYPE_NULL = 0,
    VALUE_TYPE_STRING,
    VALUE_TYPE_NUMBER,
    VALUE_TYPE_BOOLEAN
};

struct NoneType {
    bool operator==(const NoneType& none) { return true; }
    bool operator!=(const NoneType& none) { return false; }
};

struct Boolean {
    Boolean(bool value): value(value) {}

    bool operator==(const Boolean& rhs) {
        return value == rhs.value;
    }

    bool operator!=(const Boolean& rhs) {
        return value != rhs.value;
    }

    operator bool() const { return this->value; }

    bool value;
};

extern NoneType None;
extern Boolean True;
extern Boolean False;

typedef float Number;

class InvalidNodeType : public std::logic_error {
public:
    InvalidNodeType(const char* what):
        std::logic_error(what) {}
};

class InvalidValueType : public std::logic_error {
public:
    InvalidValueType(const char* what):
        std::logic_error(what) {}
};

class ParseError : public std::runtime_error {
public:
    ParseError(const char* what):
        std::runtime_error(what) {}
};

class Node {
private:
    Node(Node* parent, NodeType type);

public:
    Node(const Node& rhs) = delete;
    Node& operator=(const Node& rhs) = delete;

    Node(NodeType type=NODE_TYPE_DICT);
    ~Node();

    void insert(const String& key, const Boolean& boolean);
    void insert(const String& key, const Number& number);
    void insert(const String& key, const NoneType& none);
    void insert(const String& key, const String& string);
    void insert(const String& key, const char* string);
    void insert_list(const String& key);
    void insert_dict(const String& key);

    std::vector<std::string> keys() const {
        check_is_dict();
        std::vector<std::string> keys;
        for(auto& p: data_.map_) {
            keys.push_back(p.first);
        }
        return keys;
    }

    bool has_key(const String& key) const {
        check_is_dict();
        return bool(data_.map_.count(key));
    }

    Node& operator[](const char* key) {
        check_is_dict();
        return *data_.map_.at(key);
    }

    const Node& operator[](const char* key) const {
        check_is_dict();
        return *data_.map_.at(key);
    }

    Node& operator[](const String& key) {
        check_is_dict();
        return *data_.map_.at(key);
    }

    Node& operator[](const int64_t index) {
        check_is_list();
        return *data_.vector_.at(index);
    }

    Node& operator[](const int32_t index) {
        check_is_list();
        return *data_.vector_.at(index);
    }

    Node& operator[](const uint32_t index) {
        check_is_list();
        return *data_.vector_.at(index);
    }

    Boolean as_boolean() const {
        check_is_value();
        check_is_boolean();
        return value_.boolean_;
    }

    operator Number() const { check_is_value(); check_is_number(); return value_.number_; }
    operator Boolean() const { check_is_value(); check_is_boolean(); return value_.boolean_; }
    operator NoneType() const { check_is_value(); check_is_none(); return None; }
    operator String() const { check_is_value(); check_is_string(); return value_.string_; }
    operator bool() { check_is_value(); check_is_boolean(); return value_.boolean_; }
    operator int32_t() const { check_is_value(); check_is_number(); return value_.number_; }
#ifdef _arch_dreamcast
    operator int() const { check_is_value(); check_is_number(); return value_.number_; }
#endif
    operator int64_t() const { check_is_value(); check_is_number(); return value_.number_; }
    operator uint32_t() const { check_is_value(); check_is_number(); return value_.number_; }
    operator uint64_t() const { check_is_value(); check_is_number(); return value_.number_; }

    bool is_none() const {
        check_is_value();
        return (value_type_ == VALUE_TYPE_NULL);
    }

    bool is_number() const {
        check_is_value();
        return (value_type_ == VALUE_TYPE_NUMBER);
    }

    bool is_string() const {
        check_is_value();
        return (value_type_ == VALUE_TYPE_STRING);
    }

    bool is_bool() const {
        check_is_value();
        return (value_type_ == VALUE_TYPE_BOOLEAN);
    }

    void append(const String& string);
    void append(const Boolean& boolean);
    void append(const Number& number);
    void append(const NoneType& none);
    void append(const char* string);
    void append_list();
    void append_dict();

    uint32_t length() const { check_is_list(); return data_.vector_.size(); }

    Node& back() const { check_is_list(); return *data_.vector_.back(); }

    void operator=(const String& string) {
        check_is_value();
        value_type_ = VALUE_TYPE_STRING;
        new(&value_.string_) String(string);
    }

    void operator=(const Boolean& boolean) {
        check_is_value();
        value_type_ = VALUE_TYPE_BOOLEAN;
        new(&value_.boolean_) Boolean(boolean);
    }

    void operator=(const Number& number) {
        check_is_value();
        value_type_ = VALUE_TYPE_NUMBER;
        new(&value_.number_) Number(number);
    }

    void operator=(const NoneType& none) {
        check_is_value();
        value_type_ = VALUE_TYPE_NULL;
    }

    void operator=(const char* string) {
        check_is_value();
        value_type_ = VALUE_TYPE_STRING;
        new(&value_.string_) String(string);
    }

    friend void loads(const String& data, Node& node);
private:
    Node* parent_ = nullptr;

    ValueType value_type_;
    NodeType node_type_;

    union Value {
        Value():string_(String()) {}
        ~Value() {}

        String string_;
        Number number_;
        bool boolean_;
    };

    union Data {
        Data():map_(MapType()) {}
        ~Data() {}

        MapType map_;
        ListType vector_;
    } data_ ;

    Value value_;

    void check_is_dict() const {
        if(node_type_ != NODE_TYPE_DICT) {
            throw InvalidNodeType("Expected map");
        }
    }

    void check_is_list() const {
        if(node_type_ != NODE_TYPE_LIST) {
            throw InvalidNodeType("Expected list");
        }
    }

    void check_is_value() const {
        if(node_type_ != NODE_TYPE_VALUE) {
            throw InvalidNodeType("Expected value");
        }
    }

    void check_is_number() const {
        if(value_type_ != VALUE_TYPE_NUMBER) {
            throw InvalidValueType("Expected number");
        }
    }

    void check_is_boolean() const {
        if(value_type_ != VALUE_TYPE_BOOLEAN) {
            throw InvalidValueType("Expected boolean");
        }
    }

    void check_is_none() const {
        if(value_type_ != VALUE_TYPE_NULL) {
            throw InvalidValueType("Expected none");
        }
    }

    void check_is_string() const {
        if(value_type_ != VALUE_TYPE_STRING) {
            throw InvalidValueType("Expected string");
        }
    }

    template<typename ValueType>
    void do_insert(const String& key, const ValueType& value) {
        check_is_dict();

        // Create the new node
        auto new_node = new Node(this, NODE_TYPE_VALUE);
        (*new_node) = value;

        if(data_.map_.count(key)) {
            // Delete any existing key we swap then delete so
            // that make things atomic (... as we can without shared_ptr)
            std::swap(data_.map_[key], new_node);
            delete new_node;
        } else {
            // Assign the new node
            data_.map_[key] = new_node;
        }
    }

    void do_node_insert(const String& key, NodeType type) {
        check_is_dict();

        // Create the new node
        auto new_node = new Node(this, type);

        if(data_.map_.count(key)) {
            // Delete any existing key we swap then delete so
            // that make things atomic (... as we can without shared_ptr)
            std::swap(data_.map_[key], new_node);
            delete new_node;
        } else {
            // Assign the new node
            data_.map_[key] = new_node;
        }
    }

    template<typename ValueType>
    void do_append(ValueType value) {
        check_is_list();

        auto new_node = new Node(this, NODE_TYPE_VALUE);
        (*new_node) = value;
        data_.vector_.push_back(new_node);
    }

    void do_node_append(NodeType type) {
        check_is_list();

        auto new_node = new Node(this, type);
        data_.vector_.push_back(new_node);
    }
};

void loads(const String& data, Node& node);

}
