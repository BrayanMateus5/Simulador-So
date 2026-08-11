#include "memoria.h"

namespace OS
{
    int Memoria::alocar_frame()
    {
        // procura as vagas de frames
        for (uint16_t i = 0; i < total_frames; i++)
        {
            if (frame_ocupado[i] == false) // achou uma vaga livre
            {
                frame_ocupado[i] = true; // pega a vaga
                return i;                // devolve um id
            }
        }
        return -1; // não tem frame livre
    }

    void Memoria::liberar_frame(uint16_t id)
    {
        frame_ocupado[id] = false;
    }

    void Memoria::liberar_todos()
    {
        frame_ocupado = {};
    }
}