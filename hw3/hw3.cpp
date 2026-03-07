#include <iostream>
#include <bitset>
#include <cstdlib>
#include <cmath>
#include <cstdint>

using namespace std;

uint32_t floatToBits(float value)
{
    return *reinterpret_cast<uint32_t*>(&value);
}

void printFloatBits(float value)
{
    uint32_t bits = floatToBits(value);
    bitset<32> b(bits);
    std::string s = b.to_string();

    std::cout 
        << s.substr(0,1) << " "
        << s.substr(1,8) << " "
        << s.substr(9,23)
        << std::endl;
}

int getExponent(uint32_t bits)
{
    return (bits >> 23) & 0xFF;
}

float buildThreshold(int exponentDiff)
{
    int biasedExp = (exponentDiff - 2);
	
    uint32_t bits = (biasedExp << 23); // fraction = 0
    float result = *reinterpret_cast<float*>(&bits);

    return result;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cout << "Error: Program requires exactly two arguments." << endl;
        return 1;
    }

    float loopBound = atof(argv[1]);
    float increment = atof(argv[2]);

    cout << "Loop Bound:     ";
    printFloatBits(loopBound);

    cout << "Loop Increment: ";
    printFloatBits(increment);

    uint32_t boundBits = floatToBits(loopBound);
    uint32_t incBits = floatToBits(increment);

    int boundExp = getExponent(boundBits);
    int incExp = getExponent(incBits);

    int diff = boundExp - incExp;
	cout << endl;

    if (diff >= 23)
    {
        cout << "Warning: Possible overflow!" << endl;

        float threshold = buildThreshold(boundExp);

        cout << "Overflow threshold:" << endl;
        cout << threshold << endl;
        printFloatBits(threshold);
    }
    else
    {
        cout << "No overflow!" << endl;
    }

    return 0;
}