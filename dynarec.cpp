
// Copyright 2012 Matthew Brennan Jones <mattjones@workhorsy.org>
// A simpe example of how to dynamically recompile code,
// into x86 assembly then run it.

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
		default:
			return "Unknown";
	}
}

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
		_index(0) {

		// Allocate memory to hold the code.
		// Make to so we have permission to run code in it
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
				out << "Unknown register '" << reg_name(reg) << "'" << endl;
				throw EmitterException(out.str());
		}

		emit8(0x66);
		emit8(code);
		print_code(_index-2, 2);
		cout << " push " << reg << endl;
	}

	void pop(Register reg) throw(EmitterException) {
		u8 code = 0;

		switch(reg) {
			case EAX: code = 0x58; break;
			default:
				stringstream out;
				out << "Unknown register '" << reg_name(reg) << "'" << endl;
				throw EmitterException(out.str());
		}

		emit8(0x66);
		emit8(code);
		print_code(_index-2, 2);
		cout << " pop " << reg << endl;
	}

	void mov(Register reg, u8 value) throw(EmitterException) {
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
				stringstream out;
				out << "Unknown register '" << reg_name(reg) << "'" << endl;
				throw EmitterException(out.str());
		}

		emit8(0x66);
		emit8(code);
		emit8(0x00); emit8(0x00); emit8(0x00);
		print_code(_index-6, 6);
		cout << " mov " << reg << " " << value << endl;
	}

	void ret() {
		emit8(0xC3);
		print_code(_index-1, 1);
		cout << " ret" << endl;
	}

	void print_code(size_t start, size_t size) {
		// Address
		cout << "0x" << hex << (u32) start << "  ";

		// Opcode
		for(size_t j=start; j<start+size; j++) {
			if(_code[j] <= 0xF) cout << '0';
			cout << hex << (u32) _code[j];
		}
	}

	void execute() {
		void (*asm_code)() = (void (*)()) _code;
		asm_code();
	}
};




int main(int argc, char** argv) {
	try {
		// Create the emitter
		size_t code_size = 255;
		Emitter emitter(code_size);

		// Add the code
		emitter.push(EBX);
		//emitter.mov(EAX, 0xe);
		emitter.pop(EAX);
		emitter.ret();

		// Run it
		emitter.execute();

		// Check the value of the register
		register u32 eax asm("eax");
		cout << "eax: " << eax << endl;

	} catch(EmitterException& e) {
		cout << e.what() << endl;
	}

	return 0;
}


