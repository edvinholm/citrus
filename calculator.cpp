

struct Calculator
{
    int id;
    
    int input;

    int num_users; // A user is for example a calculator view. The calculator is deleted when this number reaches zero.
};

void press_calculator_digit(int digit, Calculator *calc)
{
    Assert(digit >= 0 && digit <= 9);

    u32 x = abs(calc->input);
    int number_of_digits_in_input = 0;
    do {
        x /= 10;
        number_of_digits_in_input++;
    } while(x != 0);
    
    if(number_of_digits_in_input >= 10) return;

    calc->input *= 10;
    calc->input += digit;
}

