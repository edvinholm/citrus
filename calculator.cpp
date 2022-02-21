
enum Calculator_Operator
{
    CALC_OP_NONE = 0,
    
    CALC_OP_ADD,
    CALC_OP_SUBTRACT,
    CALC_OP_MULTIPLY,
    CALC_OP_DIVIDE,

};

struct Calculator
{
    int id;
    
    int input;
    int hidden_operand;
    bool waiting_on_first_digit; // If we pressed an operand and have not received the second operand's first digit yet, this is true, and when we do receive the digit, we should reset input to zero first.
    Calculator_Operator op;
    bool operation_executed; // If this is true, we want to clear input to zero and reset operator to none before entering digits.

    int num_users; // A user is for example a calculator view. The calculator is deleted when this number reaches zero.
};

void enter_calculator_digit(int digit, Calculator *calc)
{
    Assert(digit >= 0 && digit <= 9);

    if(calc->waiting_on_first_digit) {
        calc->input = 0;
        calc->waiting_on_first_digit = false;
    }

    if(calc->operation_executed) {
        calc->input = 0;
        calc->op = CALC_OP_NONE;
        calc->operation_executed = false;
    }
    
    u32 x = abs(calc->input);
    int number_of_digits_in_input = 0;
    do {
        x /= 10;
        number_of_digits_in_input++;
    } while(x != 0);
    
    if(number_of_digits_in_input >= 9) return;


    calc->input *= 10;
    calc->input += digit;
}

bool execute_calculator_operation(Calculator *calc)
{
    if(calc->waiting_on_first_digit) return false;
    if(calc->op == CALC_OP_NONE) return false;

    int result = 0;
    int x = calc->hidden_operand;
    int y = calc->input;
    
    switch(calc->op) {
        case CALC_OP_ADD:      result = x + y; break;
        case CALC_OP_SUBTRACT: result = x - y; break;
        case CALC_OP_MULTIPLY: result = x * y; break;
        case CALC_OP_DIVIDE:   result = x / y; break;
    }

    calc->input = result;
    calc->hidden_operand = y;
    calc->operation_executed = true;

    return true;
}

void enter_calculator_operator(Calculator_Operator op, Calculator *calc)
{
    if(calc->op != CALC_OP_NONE) {
        if(calc->waiting_on_first_digit) return;
        if(!calc->operation_executed) {
            bool success = execute_calculator_operation(calc);
            Assert(success);
        }
    }

    calc->hidden_operand = calc->input;
    calc->waiting_on_first_digit = true;
    calc->operation_executed = false;
    calc->op = op;
}
