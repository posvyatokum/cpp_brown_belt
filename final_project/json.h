#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace Json {

  class Node : public std::variant<std::vector<Node>,
                            std::map<std::string, Node>,
                            double,
                            // int64_t,
                            bool,
                            std::string> {
  public:
    using variant::variant;

    const auto& AsVector() const {
      return std::get<std::vector<Node>>(*this);
    }
    const auto& AsMap() const {
      return std::get<std::map<std::string, Node>>(*this);
    }
    double AsDouble() const {
      return std::get<double>(*this);
    }
    int64_t AsInt() const {
        return int64_t(AsDouble());
    }
    bool AsBool() const {
      return std::get<bool>(*this);
    }
    const auto& AsString() const {
      return std::get<std::string>(*this);
    }
  };

  class Document {
  public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator == (const Document& document) const {
        return root == document.root;
    }

  private:
    Node root;
  };

  Document Load(std::istream& input);

  std::ostream& operator << (std::ostream& stream, const Node& node);
  std::ostream& operator << (std::ostream& stream, const Document& document);
}
