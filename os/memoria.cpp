#include "memoria.h"

namespace OS
{
    // se der true é ocupado, se der false tá livre
    static std::array<bool, total_frames> ocupado = {};

    // faz a separação do endereço virtual
    uint16_t pagina_de(const uint16_t vaddr)
    {
        return vaddr >> Config::page_size_bits;
    }
    uint16_t offset_de(const uint16_t vaddr)
    {
        return vaddr & (Config::page_size - 1);
    }

    // parte da memória física

    int alocar_frame()
    {
        for (uint16_t i = 0; i < total_frames; i++)
        {
            if (ocupado[i] == false)
            {
                ocupado[i] = true;
                return i;
            }
        }
        return -1; // não há espaço
    }

    void liberar_frame(const uint16_t frame)
    {
        ocupado[frame] = false;
    }
    uint16_t frames_livres()
    {
        uint16_t total = 0;
        for (uint16_t i = 0; i < total_frames; i++)
            if (ocupado[i] == false)
            {
                total++;
            }
        return total;
    }

    // parte de tabela de páginas
    void mapear_pagina(PageTable &tabela, const uint16_t pagina, const uint16_t frame)
    {
        tabela[pagina] = 0;                              // entrada zerada
        tabela[pagina].set(PteField::PhyFrameID, frame); // verifica qual frame
        tabela[pagina].set(PteField::Present, 1);        // tá na memória
        tabela[pagina].set(PteField::Readable, 1);       // pode ler
        tabela[pagina].set(PteField::Writable, 1);       // pode escrever
        tabela[pagina].set(PteField::Executable, 1);     // pode executar
    }
    void liberar_tabela(PageTable &tabela)
    {
        for (uint16_t p = 0; p < Config::ptes_per_table; p++)
        {
            if (tabela[p][PteField::Present] == 1)
            {
                liberar_frame(tabela[p][PteField::PhyFrameID]);
                tabela[p] = 0;
            }
        }
    }

    // Ponto de tradução
    int traduzir(const PageTable &tabela, const uint16_t vaddr)
    {
        const uint16_t pagina = pagina_de(vaddr);

        if (tabela[pagina][PteField::Present] == 0)
            return -1;

        const uint16_t frame = tabela[pagina][PteField::PhyFrameID];

        return frame * Config::page_size + offset_de(vaddr);
    }
}