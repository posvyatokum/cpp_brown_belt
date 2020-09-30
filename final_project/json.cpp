#include "json.h"

#include <iostream>

using namespace std;

namespace Json {

  Document::Document(Node root) : root(move(root)) {
  }

  const Node& Document::GetRoot() const {
    return root;
  }

  Node LoadNode(istream& input);

  Node LoadArray(istream& input) {
    vector<Node> result;

    for (char c; input >> c && c != ']'; ) {
      if (c != ',') {
        input.putback(c);
      }
      result.push_back(LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNumber(istream& input) {
      double result;
      input >> result;
      return Node(result);
      /*
      if (int64_t(result) == result) {
          return Node(int64_t(result));
      } else {
          return Node(double(result));
      }
      */
  }

  Node LoadBool(istream& input) {
      char c;
      input >> c;
      if (c == 't') {
          input.ignore(3);
          return Node(true);
      } else {
          input.ignore(4);
          return Node(false);
      }
  }

  Node LoadString(istream& input) {
    string line;
    getline(input, line, '"');
    return Node(move(line));
  }

  Node LoadDict(istream& input) {
    map<string, Node> result;

    for (char c; input >> c && c != '}'; ) {
      if (c == ',') {
        input >> c;
      }

      string key = LoadString(input).AsString();
      input >> c;
      result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
      return LoadArray(input);
    } else if (c == '{') {
      return LoadDict(input);
    } else if (c == '"') {
      return LoadString(input);
    } else {
      input.putback(c);
      if (c == 't' || c == 'f') {
          return LoadBool(input);
      } else {
          return LoadNumber(input);
      }
    }
  }

  Document Load(istream& input) {
    return Document{LoadNode(input)};
  }

  ostream& operator << (ostream& stream, const Node& node) {

      if (holds_alternative<string>(node)) {
          return stream << "\"" << node.AsString() << "\"";
      }
      if (holds_alternative<bool>(node)) {
          return stream << boolalpha << node.AsBool();
      }
      if (holds_alternative<double>(node)) {
          if (node.AsInt() == node.AsDouble()) {
              return stream << node.AsInt();
          } else {
              return stream << node.AsDouble();
          }
      }
      if (holds_alternative<map<string, Node>>(node)) {
          stream << "{\n";
          bool first = true;
          for (const auto& kv : node.AsMap()) {
              if (!first) {
                  stream << ",\n";
              }
              first = false;
              stream << "\"" << kv.first << "\"" << ": " << kv.second;
          }
          return stream << "\n}";
      }
      if (holds_alternative<vector<Node>>(node)) {
          stream << "[\n";
          bool first = true;
          for (const auto& x : node.AsVector()) {
              if (!first) {
                  stream << ",\n";
              }
              first = false;
              stream << x;
          }
          return stream << "\n]";
      }
  }

  ostream& operator << (std::ostream& stream, const Document& document) {
      return stream << document.GetRoot();
  }
}
