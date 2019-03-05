#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "cpu.h"
#include "system.h"

#define READ false
#define WRIT true

CPU_State init_state[256] = {
    BRK_1, IZXr1, INV_x, INV_x, INV_x, ZPGr1, ZPGb1, INV_x, PHP_1, IMMr1, IMP_1, INV_x, INV_x, ABSr1, ABSb1, INV_x,
    REL_1, IZYr1, INV_x, INV_x, INV_x, ZPXr1, ZPXb1, INV_x, IMP_1, ABYr1, INV_x, INV_x, INV_x, ABXr1, ABXb1, INV_x,
    JSR_1, IZXr1, INV_x, INV_x, ZPGr1, ZPGr1, ZPGb1, INV_x, PLP_1, IMMr1, IMP_1, INV_x, ABSr1, ABSr1, ABSb1, INV_x,
    REL_1, IZYr1, INV_x, INV_x, INV_x, ZPXr1, ZPXb1, INV_x, IMP_1, ABYr1, INV_x, INV_x, INV_x, ABXr1, ABXb1, INV_x,
    RTI_1, IZXr1, INV_x, INV_x, INV_x, ZPGr1, ZPGb1, INV_x, PHA_1, IMMr1, IMP_1, INV_x, ABSj1, ABSr1, ABSb1, INV_x,
    REL_1, IZYr1, INV_x, INV_x, INV_x, ZPXr1, ZPXb1, INV_x, IMP_1, ABYr1, INV_x, INV_x, INV_x, ABXr1, ABXb1, INV_x,
    RTS_1, IZXr1, INV_x, INV_x, INV_x, ZPGr1, ZPGb1, INV_x, PLA_1, IMMr1, IMP_1, INV_x, INDj1, ABSr1, ABSb1, INV_x,
    REL_1, IZYr1, INV_x, INV_x, INV_x, ZPXr1, ZPXb1, INV_x, IMP_1, ABYr1, INV_x, INV_x, INV_x, ABXr1, ABXb1, INV_x,
    INV_x, IZXw1, INV_x, INV_x, ZPGw1, ZPGw1, ZPGw1, INV_x, IMP_1, INV_x, IMP_1, INV_x, ABSw1, ABSw1, ABSw1, INV_x,
    REL_1, IZYw1, INV_x, INV_x, ZPXw1, ZPXw1, ZPYw1, INV_x, IMP_1, ABYw1, IMP_1, INV_x, INV_x, ABXw1, INV_x, INV_x,
    IMMr1, IZXr1, IMMr1, INV_x, ZPGr1, ZPGr1, ZPGr1, INV_x, IMP_1, IMMr1, IMP_1, INV_x, ABSr1, ABSr1, ABSr1, INV_x,
    REL_1, IZYr1, INV_x, INV_x, ZPXr1, ZPXr1, ZPYr1, INV_x, IMP_1, ABYr1, IMP_1, INV_x, ABXr1, ABXr1, ABYr1, INV_x,
    IMMr1, IZXr1, INV_x, INV_x, ZPGr1, ZPGr1, ZPGb1, INV_x, IMP_1, IMMr1, IMP_1, INV_x, ABSr1, ABSr1, ABSb1, INV_x,
    REL_1, IZYr1, INV_x, INV_x, INV_x, ZPXr1, ZPXb1, INV_x, IMP_1, ABYr1, INV_x, INV_x, INV_x, ABXr1, ABXb1, INV_x,
    IMMr1, IZXr1, INV_x, INV_x, ZPGr1, ZPGr1, ZPGb1, INV_x, IMP_1, IMMr1, IMP_1, INV_x, ABSr1, ABSr1, ABSb1, INV_x,
    REL_1, IZYr1, INV_x, INV_x, INV_x, ZPXr1, ZPXb1, INV_x, IMP_1, ABYr1, INV_x, INV_x, INV_x, ABXr1, ABXb1, INV_x,
};

typedef enum OpType {
    NOP,
    LDA, LDX, LDY,
    ADC, SBC, AND, ORA, EOR,
    BIT, CMP, CPX, CPY,
    STA, STX, STY,
    INC, INX, INY,
    DEC, DEX, DEY,
    ASL, LSR, ROL, ROR,
    AAL, LAR, RAL, RAR,
    TAX, TAY, TXA, TYA, TSX, TXS,
    BCC, BCS, BVC, BVS, BNE, BEQ, BMI, BPL,
    CLC, SEC, CLV, CLI, SEI, CLD, SED,
} OpType;

OpType op_type[256] = {
    NOP, ORA, NOP, NOP, NOP, ORA, ASL, NOP, NOP, ORA, AAL, NOP, NOP, ORA, ASL, NOP,
    BPL, ORA, NOP, NOP, NOP, ORA, ASL, NOP, CLC, ORA, NOP, NOP, NOP, ORA, ASL, NOP,
    NOP, AND, NOP, NOP, BIT, AND, ROL, NOP, NOP, AND, RAL, NOP, BIT, AND, ROL, NOP,
    BMI, AND, NOP, NOP, NOP, AND, ROL, NOP, SEC, AND, NOP, NOP, NOP, AND, ROL, NOP,
    NOP, EOR, NOP, NOP, NOP, EOR, LSR, NOP, NOP, EOR, LAR, NOP, NOP, EOR, LSR, NOP,
    BVC, EOR, NOP, NOP, NOP, EOR, LSR, NOP, CLI, EOR, NOP, NOP, NOP, EOR, LSR, NOP,
    NOP, ADC, NOP, NOP, NOP, ADC, ROR, NOP, NOP, ADC, RAR, NOP, NOP, ADC, ROR, NOP,
    BVS, ADC, NOP, NOP, NOP, ADC, ROR, NOP, SEI, ADC, NOP, NOP, NOP, ADC, ROR, NOP,
    NOP, STA, NOP, NOP, STY, STA, STX, NOP, DEY, NOP, TXA, NOP, STY, STA, STX, NOP,
    BCC, STA, NOP, NOP, STY, STA, STX, NOP, TYA, STA, TXS, NOP, NOP, STA, NOP, NOP,
    LDY, LDA, LDX, NOP, LDY, LDA, LDX, NOP, TAY, LDA, TAX, NOP, LDY, LDA, LDX, NOP,
    BCS, LDA, NOP, NOP, LDY, LDA, LDX, NOP, CLV, LDA, TSX, NOP, LDY, LDA, LDX, NOP,
    CPY, CMP, NOP, NOP, CPY, CMP, DEC, NOP, INY, CMP, DEX, NOP, CPY, CMP, DEC, NOP,
    BNE, CMP, NOP, NOP, NOP, CMP, DEC, NOP, CLD, CMP, NOP, NOP, NOP, CMP, DEC, NOP,
    CPX, SBC, NOP, NOP, CPX, SBC, INC, NOP, INX, SBC, NOP, NOP, CPX, SBC, INC, NOP,
    BEQ, SBC, NOP, NOP, NOP, SBC, INC, NOP, SED, SBC, NOP, NOP, NOP, SBC, INC, NOP,
};

typedef enum AddrBusState {
    PC_ab, AD_ab, PT_ab, ST_ab,
} AddrBusState;

AddrBusState addr_bus_state[INV_x] = {
    PC_ab,
    PC_ab,
    PC_ab,
    PC_ab, AD_ab,
    PC_ab, AD_ab, AD_ab,
    PC_ab, AD_ab, AD_ab,
    PC_ab, PC_ab, AD_ab,
    PC_ab, PC_ab, AD_ab, AD_ab,
    PC_ab, PC_ab, AD_ab, AD_ab,
    PC_ab, PT_ab, PT_ab, PT_ab, AD_ab,
    PC_ab, PT_ab, PT_ab, AD_ab, AD_ab,
    PC_ab, AD_ab,
    PC_ab, AD_ab, AD_ab,
    PC_ab, AD_ab, AD_ab,
    PC_ab, PC_ab, AD_ab,
    PC_ab, PC_ab, AD_ab, AD_ab,
    PC_ab, PC_ab, AD_ab, AD_ab,
    PC_ab, PT_ab, PT_ab, PT_ab, AD_ab,
    PC_ab, PT_ab, PT_ab, AD_ab, AD_ab,
    PC_ab, AD_ab, AD_ab, AD_ab,
    PC_ab, AD_ab, AD_ab, AD_ab, AD_ab,
    PC_ab, PC_ab, AD_ab, AD_ab, AD_ab,
    PC_ab, PC_ab, AD_ab, AD_ab, AD_ab, AD_ab,
    PC_ab, PC_ab,
    PC_ab, PC_ab, PT_ab, PT_ab,
    PC_ab, PC_ab, PC_ab,
    PC_ab, ST_ab,
    PC_ab, ST_ab,
    PC_ab, ST_ab, ST_ab,
    PC_ab, ST_ab, ST_ab,
    PC_ab, ST_ab, ST_ab, ST_ab, PC_ab,
    PC_ab, ST_ab, ST_ab, ST_ab, PC_ab,
    PC_ab, ST_ab, ST_ab, ST_ab, AD_ab, AD_ab,
    PC_ab, ST_ab, ST_ab, ST_ab, ST_ab,
};

bool rw_state[INV_x] = {
    READ,
    READ,
    READ,
    READ, READ,
    READ, READ, READ,
    READ, READ, READ,
    READ, READ, READ,
    READ, READ, READ, READ,
    READ, READ, READ, READ,
    READ, READ, READ, READ, READ,
    READ, READ, READ, READ, READ,
    READ, WRIT,
    READ, READ, WRIT,
    READ, READ, WRIT,
    READ, READ, WRIT,
    READ, READ, READ, WRIT,
    READ, READ, READ, WRIT,
    READ, READ, READ, READ, WRIT,
    READ, READ, READ, READ, WRIT,
    READ, READ, WRIT, WRIT,
    READ, READ, READ, WRIT, WRIT,
    READ, READ, READ, WRIT, WRIT,
    READ, READ, READ, READ, WRIT, WRIT,
    READ, READ,
    READ, READ, READ, READ,
    READ, READ, READ,
    READ, WRIT,
    READ, WRIT,
    READ, READ, READ,
    READ, READ, READ,
    READ, READ, WRIT, WRIT, READ,
    READ, READ, READ, READ, READ,
    READ, WRIT, WRIT, WRIT, READ, READ,
    READ, READ, READ, READ, READ,
};

u16 addr_bus(System *sys) {
    switch (addr_bus_state[sys->state]) {
    case PC_ab:
        return sys->PC;
    case AD_ab:
        return sys->addr_;
    case PT_ab:
        return sys->ptr_;
    case ST_ab:
        return 0x100 + sys->S;
    }
}

void set_ZN(System *sys, u8 a) {
    sys->Z = a == 0;
    sys->N = a & 0x80;
}

u8 P_reg(System *sys, bool bit4) {
    return (sys->C << 0) | (sys->Z << 1) | (sys->I << 2) | (sys->D << 3)
        | (bit4 << 4) | (1 << 5) | (sys->V << 6) | (sys->N << 7);
}

void perform_op(System *sys) {
    OpType op = op_type[sys->op_];
    switch (op) {
    case NOP:
        break;
    case LDA:
        sys->A = sys->data;
        set_ZN(sys, sys->A);
        break;
    case LDX:
        sys->X = sys->data;
        set_ZN(sys, sys->X);
        break;
    case LDY:
        sys->Y = sys->data;
        set_ZN(sys, sys->Y);
        break;
    case ADC: {
        int r = sys->A + sys->data + sys->C;
        sys->C = r & 0x100;
        sys->V = ~(sys->A ^ sys->data) & (sys->A ^ r) & 0x80;
        sys->A = r;
        set_ZN(sys, sys->A);
        break;
    }
    case SBC: {
        int r = sys->A + (u8)~sys->data + sys->C;
        sys->C = r & 0x100;
        sys->V = (sys->A ^ sys->data) & (sys->A ^ r) & 0x80;
        sys->A = r;
        set_ZN(sys, sys->A);
        break;
    }
    case AND:
        sys->A &= sys->data;
        set_ZN(sys, sys->A);
        break;
    case ORA:
        sys->A |= sys->data;
        set_ZN(sys, sys->A);
        break;
    case EOR:
        sys->A ^= sys->data;
        set_ZN(sys, sys->A);
        break;
    case BIT:
        sys->Z = (sys->A & sys->data) == 0;
        sys->N = sys->data & 0x80;
        sys->V = sys->data & 0x40;
        break;
    case CMP: {
        u8 r = sys->A - sys->data;
        sys->C = r <= sys->A;
        set_ZN(sys, r);
        break;
    }
    case CPX: {
        u8 r = sys->X - sys->data;
        sys->C = r <= sys->X;
        set_ZN(sys, r);
        break;
    }
    case CPY: {
        u8 r = sys->Y - sys->data;
        sys->C = r <= sys->Y;
        set_ZN(sys, r);
        break;
    }
    case STA:
        sys->data = sys->A;
        break;
    case STX:
        sys->data = sys->X;
        break;
    case STY:
        sys->data = sys->Y;
        break;
    case INC:
        sys->data++;
        set_ZN(sys, sys->data);
        break;
    case INX:
        sys->X++;
        set_ZN(sys, sys->X);
        break;
    case INY:
        sys->Y++;
        set_ZN(sys, sys->Y);
        break;
    case DEC:
        sys->data--;
        set_ZN(sys, sys->data);
        break;
    case DEX:
        sys->X--;
        set_ZN(sys, sys->X);
        break;
    case DEY:
        sys->Y--;
        set_ZN(sys, sys->Y);
        break;
    case ASL:
        sys->C = sys->data & 0x80;
        sys->data <<= 1;
        set_ZN(sys, sys->data);
        break;
    case LSR:
        sys->C = sys->data & 0x01;
        sys->data >>= 1;
        set_ZN(sys, sys->data);
        break;
    case ROL: {
        bool old_C = sys->C;
        sys->C = sys->data & 0x80;
        sys->data = (sys->data << 1) + old_C;
        set_ZN(sys, sys->data);
        break;
    }
    case ROR: {
        bool old_C = sys->C;
        sys->C = sys->data & 0x01;
        sys->data = (sys->data >> 1) + old_C * 0x80;
        set_ZN(sys, sys->data);
        break;
    }
    case AAL:
        sys->C = sys->A & 0x80;
        sys->A <<= 1;
        set_ZN(sys, sys->A);
        break;
    case LAR:
        sys->C = sys->A & 0x01;
        sys->A >>= 1;
        set_ZN(sys, sys->A);
        break;
    case RAL: {
        bool old_C = sys->C;
        sys->C = sys->A & 0x80;
        sys->A = (sys->A << 1) + old_C;
        set_ZN(sys, sys->A);
        break;
    }
    case RAR: {
        bool old_C = sys->C;
        sys->C = sys->A & 0x01;
        sys->A = (sys->A >> 1) + old_C * 0x80;
        set_ZN(sys, sys->A);
        break;
    }
    case TAX:
        sys->X = sys->A;
        set_ZN(sys, sys->X);
        break;
    case TAY:
        sys->Y = sys->A;
        set_ZN(sys, sys->Y);
        break;
    case TXA:
        sys->A = sys->X;
        set_ZN(sys, sys->A);
        break;
    case TYA:
        sys->A = sys->Y;
        set_ZN(sys, sys->A);
        break;
    case TSX:
        sys->X = sys->S;
        set_ZN(sys, sys->X);
        break;
    case TXS:
        sys->S = sys->X;
        break;
    case CLC:
        sys->C = false;
        break;
    case SEC:
        sys->C = true;
        break;
    case CLV:
        sys->V = false;
        break;
    case CLI:
        sys->I = false;
        break;
    case SEI:
        sys->I = true;
        break;
    case CLD:
        sys->D = false;
        break;
    case SED:
        sys->D = true;
        break;
    default:
        eprintln("Internal error: $%02X is a branch instruction", sys->op_);
        exit(1);
    }
}

bool check_branch(System *sys) {
    OpType op = op_type[sys->op_];
    switch (op) {
    case BCC:
        return !sys->C;
    case BCS:
        return sys->C;
    case BVC:
        return !sys->V;
    case BVS:
        return sys->V;
    case BNE:
        return !sys->Z;
    case BEQ:
        return sys->Z;
    case BPL:
        return !sys->N;
    case BMI:
        return sys->N;
    default:
        eprintln("Internal error: $%02X is not a branch instruction", sys->op_);
        exit(1);
    }
}

int cyc = 0;

void cpu_step(System *sys) {
    if (sys->OAMDMA_state != 512) {
        if (sys->OAMDMA_state < 0)
            sys->data = cpu_read(sys, sys->OAMDMA_addr);
        else if (sys->OAMDMA_state % 2 == 0)
            sys->data = cpu_read(sys, sys->OAMDMA_addr);
        else {
            sys->OAM[(u8)sys->OAMADDR] = sys->data;
            sys->OAMADDR++;
            sys->OAMDMA_addr++;
        }
        sys->OAMDMA_state++;
        return;
    }
    if (sys->NMI_occured) {
        sys->NMI_occured = false;
        sys->NMI_detected = true;
    }
    bool rw = rw_state[sys->state];
    u16 addr = addr_bus(sys);
    if (rw == READ)
        sys->data = cpu_read(sys, addr);
    switch (sys->state) {
    case XXX_0:
        if (sys->NMI_detected) {
            sys->op_ = 0x00;
            sys->state = BRK_1;
            break;
        }
        sys->op_ = sys->data;
        sys->PC++;
        sys->state = init_state[sys->op_];
        break;
    case IMP_1:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case IMMr1:
        perform_op(sys);
        sys->PC++;
        sys->state = XXX_0;
        break;
    case ZPGr1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPGr2;
        break;
    case ZPGr2:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ZPXr1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPXr2;
        break;
    case ZPXr2:
        sys->addr_ = (u8)(sys->addr_ + sys->X);
        sys->state = ZPXr3;
        break;
    case ZPXr3:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ZPYr1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPYr2;
        break;
    case ZPYr2:
        sys->addr_ = (u8)(sys->addr_ + sys->Y);
        sys->state = ZPYr3;
        break;
    case ZPYr3:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ABSr1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABSr2;
        break;
    case ABSr2:
        sys->addr_ |= sys->data << 8;
        sys->PC++;
        sys->state = ABSr3;
        break;
    case ABSr3:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ABXr1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABXr2;
        break;
    case ABXr2:
        sys->ptr_ = sys->addr_ + (sys->data << 8) + sys->X;
        sys->addr_ = sys->data << 8 | (u8)(sys->addr_ + sys->X);
        sys->PC++;
        sys->state = ABXr3;
        break;
    case ABXr3:
        if (sys->ptr_ == sys->addr_) {
            perform_op(sys);
            sys->state = XXX_0;
        } else {
            sys->addr_ = sys->ptr_;
            sys->state = ABXr4;
        }
        break;
    case ABXr4:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ABYr1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABYr2;
        break;
    case ABYr2:
        sys->ptr_ = sys->addr_ + (sys->data << 8) + sys->Y;
        sys->addr_ = sys->data << 8 | (u8)(sys->addr_ + sys->Y);
        sys->PC++;
        sys->state = ABYr3;
        break;
    case ABYr3:
        if (sys->ptr_ == sys->addr_) {
            perform_op(sys);
            sys->state = XXX_0;
        } else {
            sys->addr_ = sys->ptr_;
            sys->state = ABYr4;
        }
        break;
    case ABYr4:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case IZXr1:
        sys->ptr_ = sys->data;
        sys->PC++;
        sys->state = IZXr2;
        break;
    case IZXr2:
        sys->ptr_ = (u8)(sys->ptr_ + sys->X);
        sys->state = IZXr3;
        break;
    case IZXr3:
        sys->addr_ = sys->data;
        sys->ptr_ = (u8)(sys->ptr_ + 1);
        sys->state = IZXr4;
        break;
    case IZXr4:
        sys->addr_ |= sys->data << 8;
        sys->state = IZXr5;
        break;
    case IZXr5:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case IZYr1:
        sys->ptr_ = sys->data;
        sys->PC++;
        sys->state = IZYr2;
        break;
    case IZYr2:
        sys->addr_ = sys->data;
        sys->ptr_ = (u8)(sys->ptr_ + 1);
        sys->state = IZYr3;
        break;
    case IZYr3:
        sys->ptr_ = sys->addr_ + (sys->data << 8) + sys->Y;
        sys->addr_ = (sys->data << 8) + (u8)(sys->addr_ + sys->Y);
        sys->state = IZYr4;
        break;
    case IZYr4:
        if (sys->ptr_ == sys->addr_) {
            perform_op(sys);
            sys->state = XXX_0;
        } else {
            sys->addr_ = sys->ptr_;
            sys->state = IZYr5;
        }
        break;
    case IZYr5:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ZPGw1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPGw2;
        break;
    case ZPGw2:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ZPXw1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPXw2;
        break;
    case ZPXw2:
        sys->addr_ = (u8)(sys->addr_ + sys->X);
        sys->state = ZPXw3;
        break;
    case ZPXw3:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ZPYw1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPYw2;
        break;
    case ZPYw2:
        sys->addr_ = (u8)(sys->addr_ + sys->Y);
        sys->state = ZPYw3;
        break;
    case ZPYw3:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ABSw1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABSw2;
        break;
    case ABSw2:
        sys->addr_ |= sys->data << 8;
        sys->PC++;
        sys->state = ABSw3;
        break;
    case ABSw3:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ABXw1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABXw2;
        break;
    case ABXw2:
        sys->ptr_ = sys->addr_ + (sys->data << 8) + sys->X;
        sys->addr_ = sys->data << 8 | (u8)(sys->addr_ + sys->X);
        sys->PC++;
        sys->state = ABXw3;
        break;
    case ABXw3:
        sys->addr_ = sys->ptr_;
        sys->state = ABXw4;
        break;
    case ABXw4:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ABYw1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABYw2;
        break;
    case ABYw2:
        sys->ptr_ = sys->addr_ + (sys->data << 8) + sys->Y;
        sys->addr_ = sys->data << 8 | (u8)(sys->addr_ + sys->Y);
        sys->PC++;
        sys->state = ABYw3;
        break;
    case ABYw3:
        sys->addr_ = sys->ptr_;
        sys->state = ABYw4;
        break;
    case ABYw4:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case IZXw1:
        sys->ptr_ = sys->data;
        sys->PC++;
        sys->state = IZXw2;
        break;
    case IZXw2:
        sys->ptr_ = (u8)(sys->ptr_ + sys->X);
        sys->state = IZXw3;
        break;
    case IZXw3:
        sys->addr_ = sys->data;
        sys->ptr_ = (u8)(sys->ptr_ + 1);
        sys->state = IZXw4;
        break;
    case IZXw4:
        sys->addr_ |= sys->data << 8;
        sys->state = IZXw5;
        break;
    case IZXw5:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case IZYw1:
        sys->ptr_ = sys->data;
        sys->PC++;
        sys->state = IZYw2;
        break;
    case IZYw2:
        sys->addr_ = sys->data;
        sys->ptr_ = (u8)(sys->ptr_ + 1);
        sys->state = IZYw3;
        break;
    case IZYw3:
        sys->ptr_ = sys->addr_ + (sys->data << 8) + sys->Y;
        sys->addr_ = (sys->data << 8) + (u8)(sys->addr_ + sys->Y);
        sys->state = IZYw4;
        break;
    case IZYw4:
        sys->addr_ = sys->ptr_;
        sys->state = IZYw5;
        break;
    case IZYw5:
        perform_op(sys);
        sys->state = XXX_0;
        break;
    case ZPGb1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPGb2;
        break;
    case ZPGb2:
        sys->state = ZPGb3;
        break;
    case ZPGb3:
        perform_op(sys);
        sys->state = ZPGb4;
        break;
    case ZPGb4:
        sys->state = XXX_0;
        break;
    case ZPXb1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ZPXb2;
        break;
    case ZPXb2:
        sys->addr_ = (u8)(sys->addr_ + sys->X);
        sys->state = ZPXb3;
        break;
    case ZPXb3:
        sys->state = ZPXb4;
        break;
    case ZPXb4:
        perform_op(sys);
        sys->state = ZPXb5;
        break;
    case ZPXb5:
        sys->state = XXX_0;
        break;
    case ABSb1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABSb2;
        break;
    case ABSb2:
        sys->addr_ |= sys->data << 8;
        sys->PC++;
        sys->state = ABSb3;
        break;
    case ABSb3:
        sys->state = ABSb4;
        break;
    case ABSb4:
        perform_op(sys);
        sys->state = ABSb5;
        break;
    case ABSb5:
        sys->state = XXX_0;
        break;
    case ABXb1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABXb2;
        break;
    case ABXb2:
        sys->ptr_ = sys->addr_ + (sys->data << 8) + sys->X;
        sys->addr_ = sys->data << 8 | (u8)(sys->addr_ + sys->X);
        sys->PC++;
        sys->state = ABXb3;
        break;
    case ABXb3:
        sys->addr_ = sys->ptr_;
        sys->state = ABXb4;
        break;
    case ABXb4:
        sys->state = ABXb5;
        break;
    case ABXb5:
        perform_op(sys);
        sys->state = ABXb6;
        break;
    case ABXb6:
        sys->state = XXX_0;
        break;
    case ABSj1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = ABSj2;
        break;
    case ABSj2:
        sys->PC = (sys->data << 8) | sys->addr_;
        sys->state = XXX_0;
        break;
    case INDj1:
        sys->ptr_ = sys->data;
        sys->PC++;
        sys->state = INDj2;
        break;
    case INDj2:
        sys->ptr_ |= sys->data << 8;
        sys->PC++;
        sys->state = INDj3;
        break;
    case INDj3:
        sys->addr_ = sys->data;
        sys->ptr_ = (sys->ptr_ & 0xFF00) | (u8)(sys->ptr_ + 1);
        sys->state = INDj4;
        break;
    case INDj4:
        sys->PC = (sys->data << 8) | sys->addr_;
        sys->state = XXX_0;
        break;
    case REL_1:
        sys->PC++;
        if (check_branch(sys)) {
            sys->addr_ = (sys->PC & 0xFF00) + (u8)(sys->PC + sys->data);
            sys->ptr_ = sys->PC + (int8_t)sys->data;
            sys->state = REL_2;
        } else {
            sys->state = XXX_0;
        }
        break;
    case REL_2:
        sys->PC = sys->addr_;
        sys->state = sys->ptr_ == sys->PC ? XXX_0 : REL_3;
        break;
    case REL_3:
        sys->PC = sys->ptr_;
        sys->state = XXX_0;
        break;
    case PHA_1:
        sys->state = PHA_2;
        break;
    case PHA_2:
        sys->data = sys->A;
        sys->S--;
        sys->state = XXX_0;
        break;
    case PHP_1:
        sys->state = PHP_2;
        break;
    case PHP_2:
        sys->data = P_reg(sys, true);
        sys->S--;
        sys->state = XXX_0;
        break;
    case PLA_1:
        sys->state = PLA_2;
        break;
    case PLA_2:
        sys->S++;
        sys->state = PLA_3;
        break;
    case PLA_3:
        sys->A = sys->data;
        set_ZN(sys, sys->A);
        sys->state = XXX_0;
        break;
    case PLP_1:
        sys->state = PLP_2;
        break;
    case PLP_2:
        sys->S++;
        sys->state = PLP_3;
        break;
    case PLP_3:
        sys->C = sys->data & 0x01;
        sys->Z = sys->data & 0x02;
        sys->I = sys->data & 0x04;
        sys->D = sys->data & 0x08;
        sys->V = sys->data & 0x40;
        sys->N = sys->data & 0x80;
        sys->state = XXX_0;
        break;
    case JSR_1:
        sys->addr_ = sys->data;
        sys->PC++;
        sys->state = JSR_2;
        break;
    case JSR_2:
        sys->state = JSR_3;
        break;
    case JSR_3:
        sys->data = sys->PC >> 8;
        sys->S--;
        sys->state = JSR_4;
        break;
    case JSR_4:
        sys->data = sys->PC;
        sys->S--;
        sys->state = JSR_5;
        break;
    case JSR_5:
        sys->PC = (sys->data << 8) | sys->addr_;
        sys->state = XXX_0;
        break;
    case RTS_1:
        sys->state = RTS_2;
        break;
    case RTS_2:
        sys->S++;
        sys->state = RTS_3;
        break;
    case RTS_3:
        sys->PC = (sys->PC & 0xFF00) | sys->data;
        sys->S++;
        sys->state = RTS_4;
        break;
    case RTS_4:
        sys->PC = (sys->PC & 0x00FF) | sys->data << 8;
        sys->state = RTS_5;
        break;
    case RTS_5:
        sys->PC++;
        sys->state = XXX_0;
        break;
    case BRK_1:
        sys->state = BRK_2;
        break;
    case BRK_2:
        sys->data = sys->PC >> 8;
        sys->S--;
        sys->state = BRK_3;
        break;
    case BRK_3:
        sys->data = sys->PC;
        sys->S--;
        sys->state = BRK_4;
        break;
    case BRK_4:
        sys->data = P_reg(sys, true);
        if (sys->NMI_detected) {
            sys->NMI_detected = false;
            sys->addr_ = 0xFFFA;
        } else {
            sys->addr_ = 0xFFFE;
        }
        sys->S--;
        sys->state = BRK_5;
        break;
    case BRK_5:
        sys->PC = (sys->PC & 0xFF00) | sys->data;
        sys->addr_++;
        sys->state = BRK_6;
        break;
    case BRK_6:
        sys->PC = (sys->PC & 0x00FF) | sys->data << 8;
        sys->state = XXX_0;
        break;
    case RTI_1:
        sys->state = RTI_2;
        break;
    case RTI_2:
        sys->S++;
        sys->state = RTI_3;
        break;
    case RTI_3:
        sys->C = sys->data & 0x01;
        sys->Z = sys->data & 0x02;
        sys->I = sys->data & 0x04;
        sys->D = sys->data & 0x08;
        sys->V = sys->data & 0x40;
        sys->N = sys->data & 0x80;
        sys->S++;
        sys->state = RTI_4;
        break;
    case RTI_4:
        sys->PC = (sys->PC & 0xFF00) | sys->data;
        sys->S++;
        sys->state = RTI_5;
        break;
    case RTI_5:
        sys->PC = (sys->PC & 0x00FF) | sys->data << 8;
        sys->state = XXX_0;
        break;
    case INV_x:
        eprintln("Error: invalid opcode $%02X", sys->op_);
        exit(1);
    }
    if (rw == WRIT)
        cpu_write(sys, addr, sys->data);
    cyc++;
}
