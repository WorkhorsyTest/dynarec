
// Copyright 2012 Matthew Brennan Jones <mattjones@workhorsy.org>
// A simpe example of how to dynamically recompile code,
// into x86 assembly, then run it.

// FIXME: Add a cache that stores the already converted instructions by the source address.
// Make sure to recompile into blocks and stop at breaks and dynamically changed code.
// nacl_dyncode_create
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


typedef enum {
	EAX,
	EBX,
	ECX,
	EDX,
	ESI,
	EDI,
	EBP,
	ESP
} Register;


string reg_name(Register reg) {
	switch(reg) {
		case EAX: return "EAX";
		case EBX: return "EBX";
		case ECX: return "ECX";
		case EDX: return "EDX";
		case ESI: return "ESI";
		case EDI: return "EDI";
		case EBP: return "EBP";
		case ESP: return "ESP";
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
		if(_code_index + additional_size >= _CODE_SIZE) {
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
		_code(NULL),
		_CODE_SIZE(code_size),
		_code_index(0),
		_instruction_start(0) {

		// Allocate memory to hold the code. The mmap function is needed 
		// to give us permission to run code from inside the code block.
		_code = (u8*) mmap(
					NULL,
					_CODE_SIZE,
					PROT_EXEC | PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS,
					0,
					0);
	}

	~Emitter() {
		if(_code != NULL) {
			munmap(_code, _CODE_SIZE);
			_code = NULL;
		}
	}

	void push(Register reg) throw(EmitterException) {
		reset_instruction_start();
		u8 code = 0;

		switch(reg) {
			case EAX: code = 0x50; break;
			case EBX: code = 0x53; break;
			case ECX: code = 0x51; break;
			case EDX: code = 0x52; break;
			case ESI: code = 0x56; break;
			case EDI: code = 0x57; break;
			case EBP: code = 0x55; break;
			case ESP: code = 0x54; break;
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

	void pop(Register reg) throw(EmitterException) {
		reset_instruction_start();
		u8 code = 0;

		switch(reg) {
			case EAX: code = 0x58; break;
			case EBX: code = 0x5B; break;
			case ECX: code = 0x59; break;
			case EDX: code = 0x5A; break;
			case ESI: code = 0x5E; break;
			case EDI: code = 0x5F; break;
			case EBP: code = 0x5D; break;
			case ESP: code = 0x5C; break;
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

	void mov(Register reg, u8 value) throw(EmitterException) {
		reset_instruction_start();
		u8 code = 0;

		switch(reg) {
			case EAX: code = 0xB8; break;
			case EBX: code = 0xBB; break;
			case ECX: code = 0xB9; break;
			case EDX: code = 0xBA; break;
			case ESI: code = 0xBE; break;
			case EDI: code = 0xBF; break;
			case EBP: code = 0xBD; break;
			case ESP: code = 0xBC; break;
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
		for(size_t i=start; i<end; i++) {
			if(_code[i] <= 0xF) cout << '0';
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
	try {
		// Create the emitter
		const size_t CODE_SIZE = 255;
		Emitter emitter(CODE_SIZE);

		// Add the code
		emitter.push(EBX);
		emitter.mov(EBX, 0xE);
		emitter.pop(EBX);
		emitter.ret();

		// Check the value of the register
		register u32 ebx_before asm("ebx");
		cout << "ebx before: " << ebx_before << endl;
		
		// Run it
		emitter.execute();

		// Check the value of the register
		register u32 ebx_after asm("ebx");
		cout << "ebx after:  " << ebx_after << endl;

	} catch(EmitterException& e) {
		cout << e.what() << endl;
		return -1;
	}

	return 0;
}


