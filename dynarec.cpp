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
#include <map>


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
	return std::map<Reg, string>({
		{ Reg::EAX, "EAX" },
		{ Reg::EBX, "EBX" },
		{ Reg::ECX, "ECX" },
		{ Reg::EDX, "EDX" },
		{ Reg::ESI, "ESI" },
		{ Reg::EDI, "EDI" },
		{ Reg::EBP, "EBP" },
		{ Reg::ESP, "ESP" },
	}).at(reg);
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

		const u8 code = std::map<Reg, u8>({
			{ Reg::EAX, 0x50 },
			{ Reg::EBX, 0x53 },
			{ Reg::ECX, 0x51 },
			{ Reg::EDX, 0x52 },
			{ Reg::ESI, 0x56 },
			{ Reg::EDI, 0x57 },
			{ Reg::EBP, 0x55 },
			{ Reg::ESP, 0x54 },
		}).at(reg);

		emit8(0x66);
		emit8(code);
		print_instruction();
		cout << "   push " << reg_name(reg) << endl;
	}

	void pop(Reg reg) throw(EmitterException) {
		reset_instruction_start();

		const u8 code = std::map<Reg, u8>({
			{ Reg::EAX, 0x58 },
			{ Reg::EBX, 0x5B },
			{ Reg::ECX, 0x59 },
			{ Reg::EDX, 0x5A },
			{ Reg::ESI, 0x5E },
			{ Reg::EDI, 0x5F },
			{ Reg::EBP, 0x5D },
			{ Reg::ESP, 0x5C },
		}).at(reg);

		emit8(0x66);
		emit8(code);
		print_instruction();
		cout << "   pop " << reg_name(reg) << endl;
	}

	void mov(Reg reg, u8 value) throw(EmitterException) {
		reset_instruction_start();

		const u8 code = std::map<Reg, u8>({
			{ Reg::EAX, 0xB8 },
			{ Reg::EBX, 0xBB },
			{ Reg::ECX, 0xB9 },
			{ Reg::EDX, 0xBA },
			{ Reg::ESI, 0xBE },
			{ Reg::EDI, 0xBF },
			{ Reg::EBP, 0xBD },
			{ Reg::ESP, 0xBC },
		}).at(reg);

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

private:
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

	u8* _code;
	const size_t _CODE_SIZE;
	size_t _code_index;
	size_t _instruction_start;
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
		return 1;
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


