.text
.globl L2
.type L2, @function
L2:
addq $-8 %rsp
moveq %r12, %r12
moveq %r13, %r13
moveq %r14, %r14
moveq %r15, %r15
moveq %rbx, %rbx
L9:
movq $10, %rdi
movq %rdi, %rdi
L6:
movq $0, none
cmpq none, %rdi
jge L7
L3:
movq $0, none
movq none, none
jmp L8
L7:
movq $0, none
movq none, none
addq none, none
pushq none
movq %rdi, %rdi
call printi
movq none, none
movq $1, none
movq %rdi, %rdi
subq none, %rdi
movq %rdi, %rdi
movq $2, none
cmpq none, %rdi
je L4
L5:
jmp L6
L4:
jmp L3
L8:
moveq %r12, %r12
moveq %r13, %r13
moveq %r14, %r14
moveq %r15, %r15
moveq %rbx, %rbx
moveq none, %rsp
ret

