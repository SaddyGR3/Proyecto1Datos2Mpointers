#ifndef NODE_H
#define NODE_H

#include "MPointers.h"
#include <iostream>

template <typename T>
struct Node {
    T value;
    MPointer<Node<T>> next;

    Node() : value(), next(nullptr) {}
    explicit Node(const T& val) : value(val), next(nullptr) {}

    friend std::ostream& operator<<(std::ostream& os, const Node<T>& node) {
        os << node.value;
        return os;
    }

    friend std::istream& operator>>(std::istream& is, Node<T>& node) {
        is >> node.value;
        return is;
    }
};

#endif // NODE_H