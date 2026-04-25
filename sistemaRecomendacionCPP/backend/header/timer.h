#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>


class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::string name;

public:
    explicit Timer(const std::string& name = "") : name(name) {
        start_time = std::chrono::high_resolution_clock::now();
    }

    // Reinicia el cronómetro
    void reset(const std::string& new_name = "") {
        name = new_name;
        start_time = std::chrono::high_resolution_clock::now();
    }

    // Retorna el tiempo en segundos desde el último reset/start
    double elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;
        return diff.count();
    }

    // Imprime el tiempo transcurrido si hay nombre
    void printElapsed(std::ostream& out,const std::string& suffix = "seg") const {
        if (!name.empty()) {
            out << "[Timer] " << name << ": " << elapsed() << " " << suffix << std::endl;
        }
    }
};
