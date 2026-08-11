#ifndef OS_MEMORIA_H
#define OS_MEMORIA_H

#include <array>
#include <cstdint>

#include "../config.h"

namespace OS
{
    class Memoria
    {
    private:
        // memória física(32768)/tamanho da página(16) = total de frames(2048)
        static constexpr uint16_t total_frames = Config::phys_mem_size_words / Config::page_size;

        // se der false tá livre, se der true tá ocupado o frame
        std::array<bool, total_frames> frame_ocupado = {};

    public:
        int alocar_frame(); // volta o id um frame livre ou -1 se tiver ocupado

        void liberar_frame(uint16_t id); // marca como frame livre
        void liberar_todos();            // libera os frames
    };
}

#endif