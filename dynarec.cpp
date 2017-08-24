/*
Copyright (c) 2014-2017 Matthew Brennan Jones <matthew.brennan.jones@gmail.com>
A simpe example of how to dynamically recompile code, into x86 assembly, then run it.
It uses the MIT License
https://github.com/WorkhorsyTest/dynarec
*/

// FIXME: Add a cache that stores the already converted instructions by the source address.
// Make sure to recompile into blocks and stop at breaks and dynamically changed code.
///*
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <sstream>
#include <assert.h>
#include <sys/mman.h>


using namespace std;

typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;


enum class Reg {
	EAX,
	EBX,
	ECX,
	EDX,
	ESI,
	EDI,
	EBP,
	ESP
};


string reg_name(Reg reg) {
	switch (reg) {
		case Reg::EAX: return "EAX";
		case Reg::EBX: return "EBX";
		case Reg::ECX: return "ECX";
		case Reg::EDX: return "EDX";
		case Reg::ESI: return "ESI";
		case Reg::EDI: return "EDI";
		case Reg::EBP: return "EBP";
		case Reg::ESP: return "ESP";
		default: return "Unknown";
	}
}

union CodeRunner {
	void* code;
	void (*call)();
};

class EmitterException : public std::exception {
	string _message;
	
public:
	EmitterException(string message) : 
		_message(message) {
	}

	~EmitterException() throw() {
	}

	virtual const char* what() const throw() { 
		return _message.c_str();
	}
};

class Emitter {
	u8* _code;
	const size_t _CODE_SIZE;
	size_t _code_index;
	size_t _instruction_start;

	void assert_code_buffer_free_space(size_t additional_size) throw(EmitterException) {
		if (_code_index + additional_size >= _CODE_SIZE) {
			stringstream out;
			out << "Code buffer ran out of space.";
			out << " Needs to be " << _CODE_SIZE + additional_size << " bytes.";
			out << " But is only " << _CODE_SIZE << " bytes." << endl;
			throw EmitterException(out.str());
		}
	}

	void reset_instruction_start() {
		_instruction_start = _code_index;
	}

	size_t instruction_size() {
		return _code_index - _instruction_start;
	}

	void emit8(u8 byte) throw(EmitterException) {
		assert_code_buffer_free_space(1);

		u8* code8 = _code + _code_index;
		*code8 = byte;
		_code_index++;
	}

	void emit16(u16 word) throw(EmitterException) {
		assert_code_buffer_free_space(2);

		u16* code16 = (u16*) (_code + _code_index);
		*code16 = word;
		_code_index += 2;
	}

	void emit32(u32 dword) throw(EmitterException) {
		assert_code_buffer_free_space(4);

		u32* code32 = (u32*) (_code + _code_index);
		*code32 = dword;
		_code_index += 4;
	}

public:
	Emitter(const size_t code_size) :
		_code(nullptr),
		_CODE_SIZE(code_size),
		_code_index(0),
		_instruction_start(0) {

		// Allocate memory to hold the code. The mmap function is needed 
		// to give us permission to run code from inside the code block.
		_code = (u8*) mmap(
					nullptr,
					_CODE_SIZE,
					PROT_EXEC | PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS,
					0,
					0);
	}

	~Emitter() {
		if (_code != nullptr) {
			munmap(_code, _CODE_SIZE);
			_code = nullptr;
		}
	}

	void push(Reg reg) throw(EmitterException) {
		reset_instruction_start();
		u8 code = 0;

		switch (reg) {
			case Reg::EAX: code = 0x50; break;
			case Reg::EBX: code = 0x53; break;
			case Reg::ECX: code = 0x51; break;
			case Reg::EDX: code = 0x52; break;
			case Reg::ESI: code = 0x56; break;
			case Reg::EDI: code = 0x57; break;
			case Reg::EBP: code = 0x55; break;
			case Reg::ESP: code = 0x54; break;
			default:
				stringstream out;
				out << "Unknown register '" << reg_name(reg) << "' for push." << endl;
				throw EmitterException(out.str());
		}

		emit8(0x66);
		emit8(code);
		print_instruction();
		cout << "   push " << reg_name(reg) << endl;
	}

	void pop(Reg reg) throw(EmitterException) {
		reset_instruction_start();
		u8 code = 0;

		switch (reg) {
			case Reg::EAX: code = 0x58; break;
			case Reg::EBX: code = 0x5B; break;
			case Reg::ECX: code = 0x59; break;
			case Reg::EDX: code = 0x5A; break;
			case Reg::ESI: code = 0x5E; break;
			case Reg::EDI: code = 0x5F; break;
			case Reg::EBP: code = 0x5D; break;
			case Reg::ESP: code = 0x5C; break;
			default:
				stringstream out;
				out << "Unknown register '" << reg_name(reg) << "' for pop." << endl;
				throw EmitterException(out.str());
		}

		emit8(0x66);
		emit8(code);
		print_instruction();
		cout << "   pop " << reg_name(reg) << endl;
	}

	void mov(Reg reg, u8 value) throw(EmitterException) {
		reset_instruction_start();
		u8 code = 0;

		switch (reg) {
			case Reg::EAX: code = 0xB8; break;
			case Reg::EBX: code = 0xBB; break;
			case Reg::ECX: code = 0xB9; break;
			case Reg::EDX: code = 0xBA; break;
			case Reg::ESI: code = 0xBE; break;
			case Reg::EDI: code = 0xBF; break;
			case Reg::EBP: code = 0xBD; break;
			case Reg::ESP: code = 0xBC; break;
			default:
				stringstream out;
				out << "Unknown register '" << reg_name(reg) << "' for mov." << endl;
				throw EmitterException(out.str());
		}

		emit8(0x66);
		emit8(code);
		emit8(value); emit8(0x00); emit8(0x00); emit8(0x00);

		print_instruction();
		cout << "   mov " << reg_name(reg) << " " << value << endl;
	}

	void ret() {
		reset_instruction_start();

		emit8(0xC3);
		print_instruction();
		cout << "   ret" << endl;
	}

	void print_instruction() {
		size_t size = instruction_size();
		size_t start = _instruction_start;
		size_t end = _instruction_start + size;

		// Print Address
		cout << "0x" << hex << (u32) start << "   ";

		// Print Instruction
		for (size_t i=start; i<end; i++) {
			if (_code[i] <= 0xF) cout << '0';
			cout << hex << (u32) _code[i];
		}
	}

	void execute() {
		CodeRunner runner;
		runner.code = _code;
		runner.call();
	}
};


int main() {
	// Create the emitter
	const size_t CODE_SIZE = 255;
	Emitter emitter(CODE_SIZE);

	// Add the code
	try {
		emitter.push(Reg::EBX);
		emitter.mov(Reg::EBX, 0xE);
		emitter.pop(Reg::EBX);
		emitter.ret();
	} catch (const EmitterException& e) {
		cout << e.what() << endl;
		return -1;
	}

	// Check the value of the register
	register u32 ebx_before asm("ebx");
	cout << "ebx before: " << ebx_before << endl;

	// Run it
	emitter.execute();

	// Check the value of the register
	register u32 ebx_after asm("ebx");
	cout << "ebx after:  " << ebx_after << endl;

	return 0;
}


