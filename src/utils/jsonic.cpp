#include <iostream>
#include <sstream>

#include "jsonic.h"

namespace jsonic {

NoneType None;
Boolean True(true);
Boolean False(false);

Node::Node(Node* parent, NodeType type):
    parent_(parent),
    node_type_(type) {

    if(type == NODE_TYPE_DICT) {
        new(&data_.map_) MapType();
    } else if(type == NODE_TYPE_LIST) {
        new(&data_.vector_) ListType();
    }
}

Node::Node(NodeType type):
    node_type_(type) {

    if(type == NODE_TYPE_VALUE) {
        throw InvalidNodeType("Expected dict or list, not value");
    }

    if(type == NODE_TYPE_DICT) {
        new(&data_.map_) MapType();
    } else if(type == NODE_TYPE_LIST) {
        new(&data_.vector_) ListType();
    }
}

Node::~Node() {
    try {
        if(node_type_ == NODE_TYPE_LIST) {
            for(uint32_t i = 0; i < data_.vector_.size(); ++i) {
                delete data_.vector_[i];
                data_.vector_[i] = nullptr;
            }
            data_.vector_.clear();
        } else if(node_type_ == NODE_TYPE_DICT) {
            for(auto p: data_.map_) {
                delete p.second;
            }
            data_.map_.clear();
        }
    } catch(...) {
        std::cerr << "An exception occurred while destructing a jsonic::Node" << std::endl;
    }
}

void Node::insert(const String& key, const Boolean& boolean) {
    do_insert(key, boolean);
}

void Node::insert(const String& key, const Number& number) {
    do_insert(key, number);
}

void Node::insert(const String& key, const NoneType& none) {
    do_insert(key, none);
}

void Node::insert(const String& key, const String& string) {
    do_insert(key, string);
}

void Node::insert(const String& key, const char* string) {
    insert(key, String(string));
}

void Node::insert_list(const String& key) {
    do_node_insert(key, NODE_TYPE_LIST);
}

void Node::insert_dict(const String& key) {
    do_node_insert(key, NODE_TYPE_DICT);
}

void Node::append(const String& string) {
    do_append(string);
}

void Node::append(const Boolean& boolean) {
    do_append(boolean);
}

void Node::append(const Number& number) {
    do_append(number);
}

void Node::append(const NoneType& none) {
    do_append(none);
}

void Node::append(const char* string) {
    append(String(string));
}

void Node::append_list() {
    do_node_append(NODE_TYPE_LIST);
}

void Node::append_dict() {
    do_node_append(NODE_TYPE_DICT);
}

void replace_all(String& s, const String& search, const String& replace ) {
    for(size_t pos = 0; ; pos += replace.length() ) {
        // Locate the substring to replace
        pos = s.find( search, pos );
        if( pos == String::npos ) break;
        // Replace by erasing and inserting
        s.erase( pos, search.length() );
        s.insert( pos, replace );
    }
}

char find_first_token(const String& json_string) {
    Vector<char> tokens;
    tokens.push_back('{');
    tokens.push_back('}');
    tokens.push_back(',');
    tokens.push_back(':');
    tokens.push_back('[');
    tokens.push_back(']');

    for(uint32_t i = 0; i < json_string.length(); ++i) {
        char c = json_string[i];

        for(uint32_t j = 0; j < tokens.size(); ++j) {
            char tok = tokens[j];
            if(c == tok) {
                return c;
            }
        }
    }

    return '\0';
}

String unescape(const String& buffer) {
    String ret = buffer;
    replace_all(ret, "\\/", "/");
    return ret;
}

bool is_whitespace(char c) {
    Vector<char> whitespace;
    whitespace.push_back('\t');
    whitespace.push_back(' ');
    whitespace.push_back('\n');
    whitespace.push_back('\r');

    for(uint32_t i = 0; i < whitespace.size(); ++i) {
        char ch = whitespace[i];
        if(ch == c) {
            return true;
        }
    }
    return false;
}

void loads(const String& json_string, Node& node) {

    auto append_value = [](Node& parent, const String& buffer, bool buffer_is_string) {
        if(buffer_is_string) {
            parent.append(buffer);
        } else {
            if(buffer == "true" || buffer == "false") {
                parent.append((buffer == "true") ? jsonic::True : jsonic::False);
            } else if(buffer == "null") {
                parent.append(jsonic::None);
            } else {
                // Number
                double tmp = ::atof(buffer.c_str());
                parent.append(Number(tmp));
            }
        }
    };

    auto insert_value = [](Node& parent, const String& key, const String& buffer, bool buffer_is_string) {
        if(buffer_is_string) {
            parent.insert(key, buffer);
        } else {
            if(buffer == "true" || buffer == "false") {
                parent.insert(key, (buffer == "true") ? jsonic::True : jsonic::False);
            } else if(buffer == "null") {
                parent.insert(key, jsonic::None);
            } else {
                // Number
                double tmp = ::atof(buffer.c_str());
                parent.insert(key, Number(tmp));
            }
        }
    };

    auto set_value = [](Node& parent, const String& buffer, bool buffer_is_string) {
        if(buffer_is_string) {
            parent = buffer;
        } else {
            if(buffer == "true" || buffer == "false") {
                parent = (buffer == "true") ? jsonic::True : jsonic::False;
            } else if(buffer == "null") {
                parent = jsonic::None;
            } else {
                // Number
                double tmp = ::atof(buffer.c_str());
                parent = Number(tmp);
            }
        }
    };


    char tok = find_first_token(json_string);
    if(tok == '{') {
        node.node_type_ = NODE_TYPE_DICT;
        new(&node.data_.map_) MapType();
    } else if(tok =='[') {
        node.node_type_ = NODE_TYPE_LIST;
        new(&node.data_.map_) ListType();
    } else {
        throw ParseError("Not a valid JSON string");
    }

    Node* current_node = &node;

    char last_token = '\0';
    int current_line = 0;
    int current_char = 0;

    bool inside_string = false;
    bool buffer_is_string = false;

    String last_key = "";
    String buffer = "";

    for(uint64_t i = 0; i < json_string.length(); ++i) {
        char c = json_string[i];

        current_char++;

        if(c == '\n') {
            current_line++;
            current_char = 0;
        }

        if(is_whitespace(c) && !inside_string) continue; //Ignore whitespace

        switch(c) {
            case '{':
                if(inside_string){
                    buffer.push_back(c);
                    continue;
                }

                if(last_token != ':' &&
                   last_token != '[' &&
                   last_token != ',' &&
                   last_token != '\0') {

                    std::stringstream ss;
                    ss << "Hit invalid '{' token on line " << current_line << " char: " << current_char << std::endl;
                    throw ParseError(ss.str().c_str());
                }

                if(last_token != '\0') {
                    //If we've hit an { and the last token was a : then we had something like
                    // "mykey" : { ...
                    // so we can now create the dictionary node and make it current
                    if(current_node->node_type_ == NODE_TYPE_LIST) {
                        current_node->append_dict();
                        current_node = &current_node->back();
                    } else if(current_node->node_type_ == NODE_TYPE_DICT) {
                        current_node->insert_dict(last_key);
                        current_node = &((*current_node)[last_key]);
                    } else {
                        std::stringstream ss;
                        ss << "Hit invalid '{' token on line " << current_line << std::endl;
                        throw ParseError(ss.str().c_str());
                    }

                    buffer = "";
                    buffer_is_string = false;
                }

                last_token = '{';
                continue;
            break;
            case '}':
                if(inside_string){
                    buffer.push_back(c);
                    continue;
                }

                if(last_token != ':' && last_token != ']' && last_token != '}' && last_token != '{') {
                    std::stringstream ss;
                    ss << "Hit invalid '}' token on line " << current_line << std::endl;
                    throw ParseError(ss.str().c_str());
                }

                if(!buffer.empty()) {
                    if(last_token == ':') {
                        insert_value(*current_node, last_key, unescape(buffer), buffer_is_string);
                    } else {
                        set_value(*current_node, unescape(buffer), buffer_is_string);
                    }
                }
                buffer = "";
                buffer_is_string = false;

                //Move out of the current node
                current_node = current_node->parent_;
                last_token = '}';
                continue;
            break;
            case '[':
                if(inside_string){
                    buffer.push_back(c);
                    continue;
                }

                if(last_token != ':' && last_token != '\0' && last_token != '[' && last_token != ',') {
                    std::stringstream ss;
                    ss << "Hit invalid '[' token on line " << current_line << std::endl;
                    throw ParseError(ss.str().c_str());
                }

                if(last_token != '\0') {
                    if(current_node->node_type_ == NODE_TYPE_LIST) {
                        current_node->append_list();
                        current_node = &current_node->back();
                    } else if (current_node->node_type_ == NODE_TYPE_DICT) {
                        current_node->insert_list(last_key);
                        current_node = &((*current_node)[last_key]);
                    } else {
                        throw ParseError("Something went badly wrong");
                    }
                }

                last_token = '[';
            break;
            case ']':
                if(inside_string){
                    buffer.push_back(c);
                    continue;
                }

                if(!buffer.empty()) {
                    append_value(*current_node, unescape(buffer), buffer_is_string);
                    buffer = "";
                    buffer_is_string = false;
                }

                current_node = current_node->parent_;
                last_token = ']';

            break;
            case ',':
                if(inside_string){
                    buffer.push_back(c);
                    continue;
                }

                if(!buffer.empty()) {
                    if(current_node->node_type_ == NODE_TYPE_DICT) {
                        insert_value(*current_node, last_key, unescape(buffer), buffer_is_string);
                    }
                    else if(current_node->node_type_ == NODE_TYPE_LIST) {
                        append_value(*current_node, unescape(buffer), buffer_is_string);
                    } else {
                        set_value(*current_node, unescape(buffer), buffer_is_string);
                    }
                    buffer = "";
                    buffer_is_string = false;
                }
                last_token = ',';
                continue;
            break;
            case ':':
                if(inside_string){
                    buffer.push_back(c);
                    continue;
                }

                last_key = unescape(buffer);
                buffer = "";
                buffer_is_string = false;
                last_token = ':';
                continue;
            break;
            case '\\': {
                c = json_string[++i]; //Get the next character
                switch(c) {
                    case '"':
                    case '/':
                    case '\\':
                        buffer.push_back(c);
                    break;
                    case 'b':
                        buffer.push_back('\b');
                    break;
                    case 'f':
                        buffer.push_back('\f');
                    break;
                    case 'n':
                        buffer.push_back('\n');
                    break;
                    case 'r':
                        buffer.push_back('\r');
                    break;
                    case 't':
                        buffer.push_back('\t');
                    break;
                    default:
                        std::cerr << "Ignoring escape character (probably unicode): " << c << std::endl;
                        break;
                    /*case 'u': {
                        std::string value;
                        for(int j = 0; j < 4; ++j) {
                            value.push_back(json_string[++i]);
                        }
                        uint32_t unicode = decode_unicode_value(value);
                        if (unicode >= 0xD800 && unicode <= 0xDBFF) {
                            ++i; //Skip '\'
                            ++i; //Skip 'u'

                            std::string value2;
                            for(int j = 0; j < 4; ++j) {
                                value2.push_back(json_string[++i]);
                            }
                            uint32_t surrogate_pair = decode_unicode_value(value2);
                            unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogate_pair & 0x3FF);
                        }
                        buffer += codepoint_to_utf8(unicode);
                    }*/
                }
            }
            break;
            case '"': {
                inside_string = !inside_string;
                if(!inside_string) {
                    buffer_is_string = true;
                }
            }
            break;
            default:
                buffer.push_back(c);
        }
    }
}

}
