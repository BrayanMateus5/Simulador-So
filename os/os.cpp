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

namespace OS
{
	static Arch::Cpu *cpu = nullptr;
	static std::string line_buffer;

	//------lê e carrega o comando do usuário para a memória física
	static void carrega_programa(const std::string &nome)
	{

		// Aqui faz a leitura do arquivo do disco para um vetor
		std::vector<uint16_t> programa = Lib::load_from_disk_to_16bit_buffer(nome);

		// Aqui faz a cópia do vetor para a memória física
		for (uint16_t i = 0; i < programa.size(); i++)
			cpu->pmem_write(i, programa[i]);

		// aponta pro endereço 1
		cpu->set_pc(1);
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
		if (cmd == "help") // a lista de comandos
		{
			terminal_println(cpu, Terminal::App, "Comandos que podem te ajudar: ");
			terminal_println(cpu, Terminal::App, "help - mostra os comandos");
			terminal_println(cpu, Terminal::App, "ola - mostra uma mensagem");
			terminal_println(cpu, Terminal::App, "clear - limpa a tela");
			terminal_println(cpu, Terminal::App, "run - carrega o programa simple-1.bin");
			terminal_println(cpu, Terminal::App, "exit - Fecha tudo");
			terminal_println(cpu, Terminal::App, "kill - mata o progresso");
		}
		else if (cmd == "ola")
		{
			terminal_println(cpu, Terminal::App, "Ola");
		}
		else if (cmd == "clear")
		{
			for (int i = 0; i < 50; i++)

				terminal_print(cpu, Terminal::App, '\n');
		}

		else if (cmd == "run")
		{
			carrega_programa("simple-1.bin");
		}
		else if (cmd == "exit")
		{
			cpu->turn_off();
		}
		else if (cmd == "kill")
		{
			terminal_println(cpu, Terminal::Kernel, "Programa finalizado");
			carrega_programa("idle.bin");
		}
		else
		{
			terminal_println(cpu, Terminal::App, "Unknown command: ", cmd);
		}
	}

	//----------------------------------------------Interrupção de código de teclado
	void interrupt(const InterruptCode interrupt)
	{
		switch (interrupt)
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
		}
	}

	void syscall()
	{
		const uint16_t servico = cpu->get_gpr(0);

		switch (servico)
		{
		case 0: // pra fechar o processo
			terminal_println(cpu, Terminal::Kernel, "Processo encerrado");
			carrega_programa("idle.bin");
			break;

		case 1: // Esse imprime uma String
		{
			uint16_t addr = cpu->get_gpr(1);
			uint16_t ch = cpu->pmem_read(addr);
			while (ch != 0)
			{
				terminal_print(cpu, Terminal::App, static_cast<char>(ch));
				addr++;
				ch = cpu->pmem_read(addr);
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