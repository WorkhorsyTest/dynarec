
// Copyright 2012 Matthew Brennan Jones <mattjones@workhorsy.org>
// A simpe example of how to dynamically recompile code,
// into x86 assembly then run it.

// FIXME: Add a cache that stores the already converted instructions by the source address.
// Make sure to recompile into blocks and stop at breaks and dynamically changed code.

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

union CodePointer {
	void* code;
	void (*func)();
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
	size_t _size;
	size_t _index;
	size_t _start;

	void assure_code_buffer_size(size_t additional_size) throw(EmitterException) {
		if(_index + additional_size >= _size) {
			stringstream out;
			out << "Code buffer ran out of space.";
			out << " Needs to be " << _size + additional_size << " bytes.";
			out << " But is only " << _size << " bytes." << endl;
			throw EmitterException(out.str());
		}
	}

public:
	Emitter(size_t code_size) :
		_code(NULL),
		_size(code_size),
		_index(0),
		_start(0) {

		// Allocate memory to hold the code. The mmap function is needed 
		// to give us permission to run code from inside the code block.
		_code = (u8*) mmap(
					NULL,
					_size,
					PROT_EXEC | PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS,
					0,
					0);
	}

	~Emitter() {
		if(_code != NULL) {
			munmap(_code, _size);
			_code = NULL;
		}
	}

	void instruction_start() {
		_start = _index;
	}

	size_t instruction_size() {
		return _index - _start;
	}

	void emit8(u8 byte) throw(EmitterException) {
		assure_code_buffer_size(1);

		u8* code8 = _code + _index;
		*code8 = byte;
//		cout << "_code[" << _index << "] = 0x" << hex << (u32) *code8 << endl;
		_index++;
	}

	void emit16(u16 word) throw(EmitterException) {
		assure_code_buffer_size(2);

		u16* code16 = (u16*) (_code + _index);
		*code16 = word;
//		cout << "_code[" << _index << "] = 0x" << hex << (u32) *code16 << endl;
		_index += 2;
	}

	void emit32(u32 dword) throw(EmitterException) {
		assure_code_buffer_size(4);

		u32* code32 = (u32*) (_code + _index);
		*code32 = dword;
//		cout << "_code[" << _index << "] = 0x" << hex << (u32) *code32 << endl;
		_index += 4;
	}

	void push(Register reg) throw(EmitterException) {
		instruction_start();
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
		instruction_print();
		cout << "   push " << reg_name(reg) << endl;
	}

	void pop(Register reg) throw(EmitterException) {
		instruction_start();
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
		instruction_print();
		cout << "   pop " << reg_name(reg) << endl;
	}

	void mov(Register reg, u8 value) throw(EmitterException) {
		instruction_start();
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
		instruction_print();
		cout << "   mov " << reg_name(reg) << " " << value << endl;
	}

	void ret() {
		instruction_start();

		emit8(0xC3);
		instruction_print();
		cout << "   ret" << endl;
	}

	void instruction_print() {
		size_t size = instruction_size();

		// Address
		cout << "0x" << hex << (u32) _start << "   ";

		// Instruction
		for(size_t j=_start; j<_start+size; j++) {
			if(_code[j] <= 0xF) cout << '0';
			cout << hex << (u32) _code[j];
		}
	}

	void execute() {
		CodePointer pointer;
		pointer.code = _code;
		pointer.func();
	}
};


int main() {
	try {
		// Create the emitter
		size_t code_size = 255;
		Emitter emitter(code_size);

		// Add the code
		emitter.push(EBX);
		emitter.mov(EBX, 0xE);
		emitter.pop(EBX);
		emitter.ret();

		// Check the value of the register
		register u32 before_ebx asm("ebx");
		cout << "before ebx: " << before_ebx << endl;
		
		// Run it
		emitter.execute();

		// Check the value of the register
		register u32 after_ebx asm("ebx");
		cout << "after ebx:  " << after_ebx << endl;

	} catch(EmitterException& e) {
		cout << e.what() << endl;
		return -1;
	}

	return 0;
}


