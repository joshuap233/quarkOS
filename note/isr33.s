sr33:
	push   %ebp
	mov    %esp,%ebp
	push   %ecx
	push   %edx
	push   %eax
	and    $0xfffffff0,%esp
	sub    $0x30,%esp
	lea    0x4(%ebp),%eax
	mov    %eax,0x1c(%esp)
	movw   $0x60,0x2a(%esp)
	movzwl 0x2a(%esp),%eax
	mov    %eax,%edx
	in     (%dx),%al
	mov    %al,0x29(%esp)
	movzbl 0x29(%esp),%eax
	nop
	movzbl %al,%eax
	sub    $0xc,%esp
	push   %eax
	cld
	call   0x102221                   ; <kb_sc_parse>
	add    $0x10,%esp
	movw   $0x20,0x2e(%esp)
	movb   $0x20,0x2d(%esp)
	movzwl 0x2e(%esp),%edx
	movzbl 0x2d(%esp),%eax
	out    %al,(%dx)
	nop
	nop
	nop
	lea    -0xc(%ebp),%esp
	pop    %eax
	pop    %edx
	pop    %ecx
	pop    %ebp
	iret

