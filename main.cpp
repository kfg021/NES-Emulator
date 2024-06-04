#include "cpu.hpp"
#include "bus.hpp"

int main(int argc, char* argv[]){
    CPU cpu(std::make_shared<Bus>());

    std::string nesTest = "nestest.nes";
    cpu.load(nesTest, 0x8000, 0x10);
    cpu.load(nesTest, 0xC000, 0x10);

    for(int i = 0; i < 10000; i++){
        cpu.executeNextInstruction();
    }

    return 0;
}