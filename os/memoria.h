#ifndef OS_MEMORIA_H
#define OS_MEMORIA_H

#include <array>
#include <cstdint>

#include "../config.h"
#include "../arch/arch.h"

namespace OS
{
    using PageTable = Arch::Cpu::PageTable; // definindo padrão do nome para somente PageTable
    using PteField = Arch::Cpu::PteField;   // definindo o padrão para PteField

    // 32768 word de memória física e 16 por página com 2048 frames..
    inline constexpr uint16_t total_frames = Config::phys_mem_size_words / Config::page_size;
    // o inline faz com que todos compartilhe da mesma constante

    // separando o virtual em páginas e offset
    uint16_t pagina_de(uint16_t vaddr);
    uint16_t offset_de(uint16_t vaddr);

    // os frames da memória física
    int alocar_frame(); // busca tentar alocar o frame ou devolve -1 se não tiver livre
    void liberar_frame(uint16_t frame);
    uint16_t frames_livres();

    // Parte da tabela de páginas
    void mapear_pagina(PageTable &tabela, uint16_t pagina, uint16_t frame);
    void liberar_tabela(PageTable &tabela);
    int traduzir(const PageTable &tabela, uint16_t vaddr); // o endereço físico ou o -1
}
#endif