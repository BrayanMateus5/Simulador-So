#include <stdexcept>
#include <string>
#include <string_view>

#include <cstdint>
#include <cstdlib>

#include "../config.h"
#include "../lib.h"
#include "../arch/arch.h"
#include "os.h"
#include "os-lib.h"
#include <vector>
#include <array>
#include "memoria.h"

namespace OS
{
	static Arch::Cpu *cpu = nullptr;
	static std::string line_buffer;
	using PteField = Arch::Cpu::PteField;

	//----------------Estrutura de processos
	enum class Estado
	{
		Executando,
		Pronto,
		Bloqueado,
		Morto
	};
	struct Process
	{
		// verifica qual programa que vai estar rodando
		std::string nome;
		Estado estado = Estado::Pronto;
		uint16_t pc = 1;
		std::array<uint16_t, 8> registradores = {};

		// ainda vai ser feita a páginação
		PageTable page_table;
	};
	static Process processo;
	static Memoria memoria;

	// faz a leitura do estado e guarda
	void salvar_contexto()
	{
		processo.pc = cpu->get_pc();
		for (int i = 0; i < 8; i++)
			processo.registradores[i] = cpu->get_gpr(i);
	}
	void restaurar_contexto()
	{
		cpu->set_pc(processo.pc);
		for (int i = 0; i < 8; i++)
			cpu->set_gpr(i, processo.registradores[i]);
	}

	//------lê e carrega o comando do usuário para a memória física
	static void carrega_programa(const std::string &nome)
	{
		// Aqui faz a leitura do arquivo do disco para um vetor
		std::vector<uint16_t> programa = Lib::load_from_disk_to_16bit_buffer(nome);

		memoria.liberar_todos();  // libera os frames do programa anterior
		processo.page_table = {}; // Limpa a tabela de páginas antes de remontar

		// informa quantas páginas o programa ocupa, aqui no caso tamanho + 15
		const uint16_t num_paginas = (programa.size() + Config::page_size - 1) / Config::page_size;

		// nas páginas ele pega o frame, faz o mapeamento e copia
		for (uint16_t pagina = 0; pagina < num_paginas; pagina++)
		{
			const int frame = memoria.alocar_frame(); // pega o frame

			// Montagem de entrada da tabela de frames
			PageTableEntry &pte = processo.page_table[pagina];
			pte.set(PteField::PhyFrameID, frame);
			pte.set(PteField::Present, 1);
			pte.set(PteField::Readable, 1);
			pte.set(PteField::Writable, 1);
			pte.set(PteField::Executable, 1);

			// faz a copia das words da página pro frame físico
			for (uint16_t offset = 0; offset < Config::page_size; offset++)
			{
				const uint32_t indice = pagina * Config::page_size + offset;		 // posição no programa
				const uint16_t endereco_fisico = frame * Config::page_size + offset; // posição na memória
				const uint16_t valor = (indice < programa.size()) ? programa[indice] : 0;
				cpu->pmem_write(endereco_fisico, valor); // copia a página pro frame
			}
		}
		// liga a memória virtual em paginação e aponta pra tabela de processos
		cpu->set_vmem_mode(VmemMode::Paging);	   // inicia a tradução por páginação
		cpu->set_page_table(&processo.page_table); // mostra a CPU qual a tabela que vai usar

		// aponta pro endereço 1
		cpu->set_pc(1);
		processo.pc = 1;
		processo.registradores = {};
	}

	// traduz o endereço virtual para o físico, usando a paginação
	static uint16_t traduzir(uint16_t vaddr)
	{
		const uint16_t pagina = vaddr >> Config::page_size_bits;				  // número das páginas
		const uint16_t offset = vaddr & (Config::page_size - 1);				  // posição dentro da página
		const uint16_t frame = processo.page_table[pagina][PteField::PhyFrameID]; // leitura do frame da tabela

		return frame * Config::page_size + offset; // cria o endereço físico
	}
	//----------------------------parte onde inicia a "máquina"
	void boot(Arch::Cpu *cpu_ptr)
	{
		cpu = cpu_ptr; // guarda o ponteiro para utilizar depois

		terminal_println(cpu, Terminal::Command, "Type commands here"); // mensagem
		terminal_println(cpu, Terminal::App, "Apps output here");		// de cada
		terminal_println(cpu, Terminal::Kernel, "Kernel output here");	// terminal
		carrega_programa("idle.bin");
	}

	// ----leitura do comando do usuário e interpretação
	static void interpretar_comando(const std::string &cmd)
	{
		// faz a separação do comando e argumento
		std::string comando = cmd;
		std::string argumento = "";

		size_t espaco = cmd.find(' ');
		if (espaco != std::string::npos) // verificou se há espaço
		{
			comando = cmd.substr(0, espaco);
			argumento = cmd.substr(espaco + 1);
		}
		if (comando == "help") // a lista de comandos
		{
			terminal_println(cpu, Terminal::App, "Comandos que podem te ajudar: ");
			terminal_println(cpu, Terminal::App, "help - mostra os comandos");
			terminal_println(cpu, Terminal::App, "ola - mostra uma mensagem");
			terminal_println(cpu, Terminal::App, "clear - limpa a tela");
			terminal_println(cpu, Terminal::App, "run - carrega o programa simple-1.bin");
			terminal_println(cpu, Terminal::App, "exit - Fecha tudo");
			terminal_println(cpu, Terminal::App, "kill - mata o progresso");
		}
		else if (comando == "ola")
		{
			terminal_println(cpu, Terminal::App, "Ola");
		}
		else if (comando == "clear")
		{
			for (int i = 0; i < 50; i++)

				terminal_print(cpu, Terminal::App, '\n');
		}

		else if (comando == "run")
		{
			if (argumento == "") // acabou digitando run sem argumento
				terminal_println(cpu, Terminal::App, "Uso: run <nome_do_programa>");
			else
			{
				carrega_programa(argumento + ".bin");
				processo.nome = argumento + ".bin";
				processo.estado = Estado::Executando;
			}
		}
		else if (comando == "exit")
		{
			cpu->turn_off();
		}
		else if (comando == "kill")
		{
			if (processo.estado == Estado::Executando)
			{
				terminal_println(cpu, Terminal::Kernel, "Programa finalizado");
				processo.estado = Estado::Morto;
				carrega_programa("idle.bin");
			}
			else
			{
				terminal_println(cpu, Terminal::Kernel, "Nenhum processo em execucao ");
			}
		}
		else
		{
			terminal_println(cpu, Terminal::App, "Unknown command: ", cmd);
		}
	}

	//----------------------------------------------Interrupção de código de teclado
	void interrupt(InterruptCode code)
	{
		salvar_contexto(); // salva o processo atual

		switch (code)
		{
		case InterruptCode::Keyboard:

		{
			const uint16_t typed = cpu->read_io(IO_Port::TerminalReadTypedChar);
			// aqui ele vai ler a tecla digitada e desarmar

			const char c = static_cast<char>(typed);
			// pega um número e transforma num char
			if (terminal_is_return(c))
			{ // aqui é onde o usuário aperta enter e a linha se completa

				interpretar_comando(line_buffer); // interpreta
				line_buffer.clear();
				// esvazia o local para um novo comando
			}
			else
			{
				// acumula no buffer
				line_buffer += c;
			}

			terminal_print(cpu, Terminal::Command, c); // printa na tela
			break;
		}
		case InterruptCode::CpuException:
		{
			const auto excecao = cpu->get_ref_cpu_exception();																 // pega os detalhes da exceção
			terminal_println(cpu, Terminal::Kernel, "Excecao da CPU no endereco ", excecao.vaddr, " - processo finalizado"); // o endereço que gerou o problema
			processo.estado = Estado::Morto;																				 // marca o processo como morto
			carrega_programa("idle.bin");																					 // mata o processo e volta pro idle-bin
			break;
		}
		}
		restaurar_contexto();
	}

	void syscall()
	{
		const uint16_t servico = cpu->get_gpr(0);

		switch (servico)
		{
		case 0: // pra fechar o processo
			terminal_println(cpu, Terminal::Kernel, "Processo encerrado");
			processo.estado = Estado::Morto;
			carrega_programa("idle.bin");
			break;

		case 1: // Esse imprime uma String
		{
			uint16_t vaddr = cpu->get_gpr(1);			   // endereço virtual
			uint16_t ch = cpu->pmem_read(traduzir(vaddr)); // traduz e lê
			while (ch != 0)
			{
				terminal_print(cpu, Terminal::App, static_cast<char>(ch));
				vaddr++;
				ch = cpu->pmem_read(traduzir(vaddr));
			}
			break;
		}
		case 2: // nova linha
			terminal_print(cpu, Terminal::App, '\n');
			break;

		case 3: // imprime um número inteiro
			terminal_print(cpu, Terminal::App, cpu->get_gpr(1));
		}
	}

} // end namespace OS