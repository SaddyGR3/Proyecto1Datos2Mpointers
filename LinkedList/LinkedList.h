#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "MPointers.h"
#include <iostream>
#include <stdexcept>
#include <memory>

class LinkedListInt {
private:
    // Cada nodo está representado por dos MPointer<int>:
    // - Uno para el valor (ID)
    // - Otro para el siguiente nodo (ID)
    MPointer<int> head_value_id;  // ID del valor del primer nodo
    MPointer<int> head_next_id;   // ID del puntero al siguiente del primer nodo
    MPointer<int> tail_value_id;  // ID del valor del último nodo
    size_t size_ = 0;

    // Crea un nuevo nodo y devuelve los IDs creados
    struct NodeIDs {
        int value_id;
        int next_id;
    };

    NodeIDs createNode(int value, int next = -1) {
        // Crear MPointer para el valor
        MPointer<int> value_ptr = MPointer<int>::New();
        *value_ptr = value;

        // Crear MPointer para el siguiente
        MPointer<int> next_ptr = MPointer<int>::New();
        *next_ptr = next;

        return {value_ptr.getId(), next_ptr.getId()};
    }

    // Obtiene el valor de un nodo
    int getNodeValue(int value_id) const {
        if (value_id == -1) throw std::runtime_error("ID de valor inválido");
        MPointer<int> value;
        value.setId(value_id);
        return *value;
    }

    // Obtiene el ID del siguiente nodo
    int getNextNodeId(int next_id) const {
        if (next_id == -1) return -1;
        MPointer<int> next;
        next.setId(next_id);
        return *next;
    }

    // Establece el siguiente nodo
    void setNextNodeId(int next_ptr_id, int next_value_id) {
        if (next_ptr_id == -1) return;
        MPointer<int> next_ptr;
        next_ptr.setId(next_ptr_id);
        *next_ptr = next_value_id;
    }

public:
    LinkedListInt() : size_(0) {
        // Inicializar todos los MPointer
        head_value_id = MPointer<int>::New();
        head_next_id = MPointer<int>::New();
        tail_value_id = MPointer<int>::New();

        // Valores iniciales
        *head_value_id = -1;
        *head_next_id = -1;
        *tail_value_id = -1;
    }

    ~LinkedListInt() { clear(); }

    void push_front(int value) {
        NodeIDs new_node = createNode(value, *head_value_id);

        if (empty()) {
            *tail_value_id = new_node.value_id;
        } else {
            // El nuevo nodo apunta al antiguo head
            setNextNodeId(new_node.next_id, *head_value_id);
        }

        // Actualizar head
        *head_value_id = new_node.value_id;
        *head_next_id = new_node.next_id;
        size_++;
    }

    void push_back(int value) {
        NodeIDs new_node = createNode(value);

        if (empty()) {
            *head_value_id = new_node.value_id;
            *head_next_id = new_node.next_id;
        } else {
            // El antiguo tail apunta al nuevo nodo
            setNextNodeId(getTailNextId(), new_node.value_id);
        }

        *tail_value_id = new_node.value_id;
        size_++;
    }

    void pop_front() {
        if (empty()) throw std::runtime_error("Lista vacía");

        if (size_ == 1) {
            *head_value_id = -1;
            *head_next_id = -1;
            *tail_value_id = -1;
        } else {
            // Mover head al siguiente nodo
            int next_id = getNextNodeId(*head_next_id);
            *head_value_id = next_id;

            // Actualizar next_id del nuevo head
            MPointer<int> new_head_next;
            new_head_next.setId(next_id + 1); // Asume que next_id está en value_id + 1
            *head_next_id = new_head_next.getId();
        }
        size_--;
    }

    int front() const {
        if (empty()) throw std::runtime_error("Lista vacía");
        return getNodeValue(*head_value_id);
    }

    int back() const {
        if (empty()) throw std::runtime_error("Lista vacía");
        return getNodeValue(*tail_value_id);
    }

    void clear() {
        while (!empty()) {
            pop_front();
        }
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void print() const {
        std::cout << "[";
        if (!empty()) {
            int current_value_id = *head_value_id;
            int current_next_id = *head_next_id;
            bool first = true;

            while (current_value_id != -1) {
                try {
                    if (!first) std::cout << ", ";
                    std::cout << getNodeValue(current_value_id);
                    first = false;

                    current_value_id = getNextNodeId(current_next_id);
                    current_next_id = current_value_id + 1; // Asume patrón de almacenamiento
                } catch (const std::exception& e) {
                    std::cerr << "\nError al imprimir: " << e.what();
                    break;
                }
            }
        }
        std::cout << "]" << std::endl;
    }

private:
    // Obtiene el ID del puntero "siguiente" del tail
    int getTailNextId() const {
        if (*tail_value_id == -1) return -1;
        return *tail_value_id + 1; // Asume que next_id está en value_id + 1
    }
};

#endif // LINKEDLIST_H