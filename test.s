

Disassembly of section .text:

00000000 <fork>:
   0:	b8 09 00 00 00       	mov    $0x9,%eax
   5:	cd 80                	int    $0x80
   7:	c3                   	ret    

00000008 <exit>:
   8:	8b 5c 24 04          	mov    0x4(%esp),%ebx
   c:	b8 0d 00 00 00       	mov    $0xd,%eax
  11:	cd 80                	int    $0x80
  13:	c3                   	ret    

00000014 <gets>:
  14:	8b 5c 24 04          	mov    0x4(%esp),%ebx
  18:	8b 4c 24 08          	mov    0x8(%esp),%ecx
  1c:	b8 01 00 00 00       	mov    $0x1,%eax
  21:	cd 80                	int    $0x80
  23:	c3                   	ret    

00000024 <puts>:
  24:	8b 5c 24 04          	mov    0x4(%esp),%ebx
  28:	8b 4c 24 08          	mov    0x8(%esp),%ecx
  2c:	b8 02 00 00 00       	mov    $0x2,%eax
  31:	cd 80                	int    $0x80
  33:	c3                   	ret    

00000034 <exec>:
  34:	8b 5c 24 04          	mov    0x4(%esp),%ebx
  38:	b8 00 00 00 00       	mov    $0x0,%eax
  3d:	cd 80                	int    $0x80
  3f:	c3                   	ret    

00000040 <cls>:
  40:	b8 0e 00 00 00       	mov    $0xe,%eax
  45:	cd 80                	int    $0x80
  47:	c3                   	ret    

00000048 <reverse>:
  48:	55                   	push   %ebp
  49:	89 e5                	mov    %esp,%ebp
  4b:	83 ec 10             	sub    $0x10,%esp
  4e:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
  55:	8b 45 0c             	mov    0xc(%ebp),%eax
  58:	89 45 f8             	mov    %eax,-0x8(%ebp)
  5b:	eb 39                	jmp    96 <reverse+0x4e>
  5d:	8b 55 08             	mov    0x8(%ebp),%edx
  60:	8b 45 fc             	mov    -0x4(%ebp),%eax
  63:	01 d0                	add    %edx,%eax
  65:	0f b6 00             	movzbl (%eax),%eax
  68:	88 45 f7             	mov    %al,-0x9(%ebp)
  6b:	8b 55 08             	mov    0x8(%ebp),%edx
  6e:	8b 45 f8             	mov    -0x8(%ebp),%eax
  71:	01 d0                	add    %edx,%eax
  73:	8b 4d 08             	mov    0x8(%ebp),%ecx
  76:	8b 55 fc             	mov    -0x4(%ebp),%edx
  79:	01 ca                	add    %ecx,%edx
  7b:	0f b6 00             	movzbl (%eax),%eax
  7e:	88 02                	mov    %al,(%edx)
  80:	8b 55 08             	mov    0x8(%ebp),%edx
  83:	8b 45 f8             	mov    -0x8(%ebp),%eax
  86:	01 c2                	add    %eax,%edx
  88:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
  8c:	88 02                	mov    %al,(%edx)
  8e:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
  92:	83 6d f8 01          	subl   $0x1,-0x8(%ebp)
  96:	8b 45 fc             	mov    -0x4(%ebp),%eax
  99:	3b 45 f8             	cmp    -0x8(%ebp),%eax
  9c:	72 bf                	jb     5d <reverse+0x15>
  9e:	90                   	nop
  9f:	90                   	nop
  a0:	c9                   	leave  
  a1:	c3                   	ret    

000000a2 <hex>:
  a2:	55                   	push   %ebp
  a3:	89 e5                	mov    %esp,%ebp
  a5:	56                   	push   %esi
  a6:	53                   	push   %ebx
  a7:	83 ec 18             	sub    $0x18,%esp
  aa:	8b 45 08             	mov    0x8(%ebp),%eax
  ad:	89 45 e0             	mov    %eax,-0x20(%ebp)
  b0:	8b 45 0c             	mov    0xc(%ebp),%eax
  b3:	89 45 e4             	mov    %eax,-0x1c(%ebp)
  b6:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
  ba:	8b 45 e0             	mov    -0x20(%ebp),%eax
  bd:	83 e0 0f             	and    $0xf,%eax
  c0:	88 45 f6             	mov    %al,-0xa(%ebp)
  c3:	8b 45 e0             	mov    -0x20(%ebp),%eax
  c6:	8b 55 e4             	mov    -0x1c(%ebp),%edx
  c9:	0f ac d0 04          	shrd   $0x4,%edx,%eax
  cd:	c1 ea 04             	shr    $0x4,%edx
  d0:	89 45 e0             	mov    %eax,-0x20(%ebp)
  d3:	89 55 e4             	mov    %edx,-0x1c(%ebp)
  d6:	0f b6 75 f6          	movzbl -0xa(%ebp),%esi
  da:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
  de:	8d 50 01             	lea    0x1(%eax),%edx
  e1:	88 55 f7             	mov    %dl,-0x9(%ebp)
  e4:	0f b6 d0             	movzbl %al,%edx
  e7:	8b 45 10             	mov    0x10(%ebp),%eax
  ea:	01 c2                	add    %eax,%edx
  ec:	0f b6 86 58 10 00 00 	movzbl 0x1058(%esi),%eax
  f3:	88 02                	mov    %al,(%edx)
  f5:	8b 45 e0             	mov    -0x20(%ebp),%eax
  f8:	80 f4 00             	xor    $0x0,%ah
  fb:	89 c1                	mov    %eax,%ecx
  fd:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 100:	80 f4 00             	xor    $0x0,%ah
 103:	89 c3                	mov    %eax,%ebx
 105:	89 d8                	mov    %ebx,%eax
 107:	09 c8                	or     %ecx,%eax
 109:	85 c0                	test   %eax,%eax
 10b:	75 ad                	jne    ba <hex+0x18>
 10d:	0f b6 55 f7          	movzbl -0x9(%ebp),%edx
 111:	8b 45 10             	mov    0x10(%ebp),%eax
 114:	01 d0                	add    %edx,%eax
 116:	c6 00 00             	movb   $0x0,(%eax)
 119:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
 11d:	83 e8 01             	sub    $0x1,%eax
 120:	50                   	push   %eax
 121:	ff 75 10             	pushl  0x10(%ebp)
 124:	e8 1f ff ff ff       	call   48 <reverse>
 129:	83 c4 08             	add    $0x8,%esp
 12c:	90                   	nop
 12d:	8d 65 f8             	lea    -0x8(%ebp),%esp
 130:	5b                   	pop    %ebx
 131:	5e                   	pop    %esi
 132:	5d                   	pop    %ebp
 133:	c3                   	ret    

00000134 <utoa>:
 134:	55                   	push   %ebp
 135:	89 e5                	mov    %esp,%ebp
 137:	57                   	push   %edi
 138:	56                   	push   %esi
 139:	83 ec 20             	sub    $0x20,%esp
 13c:	8b 45 08             	mov    0x8(%ebp),%eax
 13f:	89 45 e0             	mov    %eax,-0x20(%ebp)
 142:	8b 45 0c             	mov    0xc(%ebp),%eax
 145:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 148:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
 14c:	8b 45 e0             	mov    -0x20(%ebp),%eax
 14f:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 152:	6a 00                	push   $0x0
 154:	6a 0a                	push   $0xa
 156:	52                   	push   %edx
 157:	50                   	push   %eax
 158:	e8 73 06 00 00       	call   7d0 <__umoddi3>
 15d:	83 c4 10             	add    $0x10,%esp
 160:	8d 48 30             	lea    0x30(%eax),%ecx
 163:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
 167:	8d 50 01             	lea    0x1(%eax),%edx
 16a:	88 55 f7             	mov    %dl,-0x9(%ebp)
 16d:	0f b6 d0             	movzbl %al,%edx
 170:	8b 45 10             	mov    0x10(%ebp),%eax
 173:	01 d0                	add    %edx,%eax
 175:	89 ca                	mov    %ecx,%edx
 177:	88 10                	mov    %dl,(%eax)
 179:	8b 45 e0             	mov    -0x20(%ebp),%eax
 17c:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 17f:	6a 00                	push   $0x0
 181:	6a 0a                	push   $0xa
 183:	52                   	push   %edx
 184:	50                   	push   %eax
 185:	e8 26 05 00 00       	call   6b0 <__udivdi3>
 18a:	83 c4 10             	add    $0x10,%esp
 18d:	89 45 e0             	mov    %eax,-0x20(%ebp)
 190:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 193:	8b 45 e0             	mov    -0x20(%ebp),%eax
 196:	80 f4 00             	xor    $0x0,%ah
 199:	89 c6                	mov    %eax,%esi
 19b:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 19e:	80 f4 00             	xor    $0x0,%ah
 1a1:	89 c7                	mov    %eax,%edi
 1a3:	89 f8                	mov    %edi,%eax
 1a5:	09 f0                	or     %esi,%eax
 1a7:	85 c0                	test   %eax,%eax
 1a9:	75 a1                	jne    14c <utoa+0x18>
 1ab:	0f b6 55 f7          	movzbl -0x9(%ebp),%edx
 1af:	8b 45 10             	mov    0x10(%ebp),%eax
 1b2:	01 d0                	add    %edx,%eax
 1b4:	c6 00 00             	movb   $0x0,(%eax)
 1b7:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
 1bb:	83 e8 01             	sub    $0x1,%eax
 1be:	83 ec 08             	sub    $0x8,%esp
 1c1:	50                   	push   %eax
 1c2:	ff 75 10             	pushl  0x10(%ebp)
 1c5:	e8 7e fe ff ff       	call   48 <reverse>
 1ca:	83 c4 10             	add    $0x10,%esp
 1cd:	90                   	nop
 1ce:	8d 65 f8             	lea    -0x8(%ebp),%esp
 1d1:	5e                   	pop    %esi
 1d2:	5f                   	pop    %edi
 1d3:	5d                   	pop    %ebp
 1d4:	c3                   	ret    

000001d5 <strlen>:
 1d5:	55                   	push   %ebp
 1d6:	89 e5                	mov    %esp,%ebp
 1d8:	83 ec 10             	sub    $0x10,%esp
 1db:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 1e2:	eb 04                	jmp    1e8 <strlen+0x13>
 1e4:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
 1e8:	8b 55 08             	mov    0x8(%ebp),%edx
 1eb:	8b 45 fc             	mov    -0x4(%ebp),%eax
 1ee:	01 d0                	add    %edx,%eax
 1f0:	0f b6 00             	movzbl (%eax),%eax
 1f3:	84 c0                	test   %al,%al
 1f5:	75 ed                	jne    1e4 <strlen+0xf>
 1f7:	8b 45 fc             	mov    -0x4(%ebp),%eax
 1fa:	c9                   	leave  
 1fb:	c3                   	ret    

000001fc <put_string>:
 1fc:	55                   	push   %ebp
 1fd:	89 e5                	mov    %esp,%ebp
 1ff:	83 ec 08             	sub    $0x8,%esp
 202:	83 ec 08             	sub    $0x8,%esp
 205:	6a ff                	push   $0xffffffff
 207:	ff 75 08             	pushl  0x8(%ebp)
 20a:	e8 15 fe ff ff       	call   24 <puts>
 20f:	83 c4 10             	add    $0x10,%esp
 212:	90                   	nop
 213:	c9                   	leave  
 214:	c3                   	ret    

00000215 <put_char>:
 215:	55                   	push   %ebp
 216:	89 e5                	mov    %esp,%ebp
 218:	83 ec 18             	sub    $0x18,%esp
 21b:	8b 45 08             	mov    0x8(%ebp),%eax
 21e:	88 45 f4             	mov    %al,-0xc(%ebp)
 221:	83 ec 08             	sub    $0x8,%esp
 224:	6a 01                	push   $0x1
 226:	8d 45 f4             	lea    -0xc(%ebp),%eax
 229:	50                   	push   %eax
 22a:	e8 f5 fd ff ff       	call   24 <puts>
 22f:	83 c4 10             	add    $0x10,%esp
 232:	90                   	nop
 233:	c9                   	leave  
 234:	c3                   	ret    

00000235 <print_d>:
 235:	55                   	push   %ebp
 236:	89 e5                	mov    %esp,%ebp
 238:	83 ec 38             	sub    $0x38,%esp
 23b:	8b 45 08             	mov    0x8(%ebp),%eax
 23e:	89 45 d0             	mov    %eax,-0x30(%ebp)
 241:	8b 45 0c             	mov    0xc(%ebp),%eax
 244:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 247:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
 24b:	83 7d d4 00          	cmpl   $0x0,-0x2c(%ebp)
 24f:	79 1c                	jns    26d <print_d+0x38>
 251:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
 255:	8d 50 01             	lea    0x1(%eax),%edx
 258:	88 55 f7             	mov    %dl,-0x9(%ebp)
 25b:	0f b6 c0             	movzbl %al,%eax
 25e:	c6 44 05 e1 2d       	movb   $0x2d,-0x1f(%ebp,%eax,1)
 263:	f7 5d d0             	negl   -0x30(%ebp)
 266:	83 55 d4 00          	adcl   $0x0,-0x2c(%ebp)
 26a:	f7 5d d4             	negl   -0x2c(%ebp)
 26d:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
 271:	8d 55 e1             	lea    -0x1f(%ebp),%edx
 274:	8d 0c 02             	lea    (%edx,%eax,1),%ecx
 277:	8b 45 d0             	mov    -0x30(%ebp),%eax
 27a:	8b 55 d4             	mov    -0x2c(%ebp),%edx
 27d:	83 ec 04             	sub    $0x4,%esp
 280:	51                   	push   %ecx
 281:	52                   	push   %edx
 282:	50                   	push   %eax
 283:	e8 ac fe ff ff       	call   134 <utoa>
 288:	83 c4 10             	add    $0x10,%esp
 28b:	83 ec 0c             	sub    $0xc,%esp
 28e:	8d 45 e1             	lea    -0x1f(%ebp),%eax
 291:	50                   	push   %eax
 292:	e8 65 ff ff ff       	call   1fc <put_string>
 297:	83 c4 10             	add    $0x10,%esp
 29a:	90                   	nop
 29b:	c9                   	leave  
 29c:	c3                   	ret    

0000029d <print_u>:
 29d:	55                   	push   %ebp
 29e:	89 e5                	mov    %esp,%ebp
 2a0:	83 ec 38             	sub    $0x38,%esp
 2a3:	8b 45 08             	mov    0x8(%ebp),%eax
 2a6:	89 45 d0             	mov    %eax,-0x30(%ebp)
 2a9:	8b 45 0c             	mov    0xc(%ebp),%eax
 2ac:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 2af:	83 ec 04             	sub    $0x4,%esp
 2b2:	8d 45 e3             	lea    -0x1d(%ebp),%eax
 2b5:	50                   	push   %eax
 2b6:	ff 75 d4             	pushl  -0x2c(%ebp)
 2b9:	ff 75 d0             	pushl  -0x30(%ebp)
 2bc:	e8 73 fe ff ff       	call   134 <utoa>
 2c1:	83 c4 10             	add    $0x10,%esp
 2c4:	83 ec 0c             	sub    $0xc,%esp
 2c7:	8d 45 e3             	lea    -0x1d(%ebp),%eax
 2ca:	50                   	push   %eax
 2cb:	e8 2c ff ff ff       	call   1fc <put_string>
 2d0:	83 c4 10             	add    $0x10,%esp
 2d3:	90                   	nop
 2d4:	c9                   	leave  
 2d5:	c3                   	ret    

000002d6 <print_pointer>:
 2d6:	55                   	push   %ebp
 2d7:	89 e5                	mov    %esp,%ebp
 2d9:	53                   	push   %ebx
 2da:	83 ec 24             	sub    $0x24,%esp
 2dd:	c7 45 e1 30 78 00 00 	movl   $0x7830,-0x1f(%ebp)
 2e4:	c7 45 e5 00 00 00 00 	movl   $0x0,-0x1b(%ebp)
 2eb:	c7 45 e9 00 00 00 00 	movl   $0x0,-0x17(%ebp)
 2f2:	c7 45 ed 00 00 00 00 	movl   $0x0,-0x13(%ebp)
 2f9:	c7 45 f1 00 00 00 00 	movl   $0x0,-0xf(%ebp)
 300:	66 c7 45 f5 00 00    	movw   $0x0,-0xb(%ebp)
 306:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
 30a:	8d 45 e1             	lea    -0x1f(%ebp),%eax
 30d:	83 c0 02             	add    $0x2,%eax
 310:	8b 55 08             	mov    0x8(%ebp),%edx
 313:	89 d1                	mov    %edx,%ecx
 315:	bb 00 00 00 00       	mov    $0x0,%ebx
 31a:	83 ec 04             	sub    $0x4,%esp
 31d:	50                   	push   %eax
 31e:	53                   	push   %ebx
 31f:	51                   	push   %ecx
 320:	e8 7d fd ff ff       	call   a2 <hex>
 325:	83 c4 10             	add    $0x10,%esp
 328:	83 ec 0c             	sub    $0xc,%esp
 32b:	8d 45 e1             	lea    -0x1f(%ebp),%eax
 32e:	50                   	push   %eax
 32f:	e8 c8 fe ff ff       	call   1fc <put_string>
 334:	83 c4 10             	add    $0x10,%esp
 337:	90                   	nop
 338:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 33b:	c9                   	leave  
 33c:	c3                   	ret    

0000033d <print_hex>:
 33d:	55                   	push   %ebp
 33e:	89 e5                	mov    %esp,%ebp
 340:	83 ec 38             	sub    $0x38,%esp
 343:	8b 45 08             	mov    0x8(%ebp),%eax
 346:	89 45 d0             	mov    %eax,-0x30(%ebp)
 349:	8b 45 0c             	mov    0xc(%ebp),%eax
 34c:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 34f:	c7 45 e1 30 78 00 00 	movl   $0x7830,-0x1f(%ebp)
 356:	c7 45 e5 00 00 00 00 	movl   $0x0,-0x1b(%ebp)
 35d:	c7 45 e9 00 00 00 00 	movl   $0x0,-0x17(%ebp)
 364:	c7 45 ed 00 00 00 00 	movl   $0x0,-0x13(%ebp)
 36b:	c7 45 f1 00 00 00 00 	movl   $0x0,-0xf(%ebp)
 372:	66 c7 45 f5 00 00    	movw   $0x0,-0xb(%ebp)
 378:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
 37c:	8d 45 e1             	lea    -0x1f(%ebp),%eax
 37f:	83 c0 02             	add    $0x2,%eax
 382:	83 ec 04             	sub    $0x4,%esp
 385:	50                   	push   %eax
 386:	ff 75 d4             	pushl  -0x2c(%ebp)
 389:	ff 75 d0             	pushl  -0x30(%ebp)
 38c:	e8 11 fd ff ff       	call   a2 <hex>
 391:	83 c4 10             	add    $0x10,%esp
 394:	83 ec 0c             	sub    $0xc,%esp
 397:	8d 45 e1             	lea    -0x1f(%ebp),%eax
 39a:	50                   	push   %eax
 39b:	e8 5c fe ff ff       	call   1fc <put_string>
 3a0:	83 c4 10             	add    $0x10,%esp
 3a3:	90                   	nop
 3a4:	c9                   	leave  
 3a5:	c3                   	ret    

000003a6 <printf>:
 3a6:	55                   	push   %ebp
 3a7:	89 e5                	mov    %esp,%ebp
 3a9:	83 ec 18             	sub    $0x18,%esp
 3ac:	ff 75 08             	pushl  0x8(%ebp)
 3af:	e8 21 fe ff ff       	call   1d5 <strlen>
 3b4:	83 c4 04             	add    $0x4,%esp
 3b7:	89 45 f0             	mov    %eax,-0x10(%ebp)
 3ba:	8d 45 0c             	lea    0xc(%ebp),%eax
 3bd:	89 45 ec             	mov    %eax,-0x14(%ebp)
 3c0:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 3c7:	e9 c9 01 00 00       	jmp    595 <printf+0x1ef>
 3cc:	8b 55 08             	mov    0x8(%ebp),%edx
 3cf:	8b 45 f4             	mov    -0xc(%ebp),%eax
 3d2:	01 d0                	add    %edx,%eax
 3d4:	0f b6 00             	movzbl (%eax),%eax
 3d7:	3c 25                	cmp    $0x25,%al
 3d9:	0f 85 98 01 00 00    	jne    577 <printf+0x1d1>
 3df:	8b 45 f4             	mov    -0xc(%ebp),%eax
 3e2:	83 c0 01             	add    $0x1,%eax
 3e5:	39 45 f0             	cmp    %eax,-0x10(%ebp)
 3e8:	0f 86 89 01 00 00    	jbe    577 <printf+0x1d1>
 3ee:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 3f2:	8b 55 08             	mov    0x8(%ebp),%edx
 3f5:	8b 45 f4             	mov    -0xc(%ebp),%eax
 3f8:	01 d0                	add    %edx,%eax
 3fa:	0f b6 00             	movzbl (%eax),%eax
 3fd:	0f be c0             	movsbl %al,%eax
 400:	83 e8 63             	sub    $0x63,%eax
 403:	83 f8 15             	cmp    $0x15,%eax
 406:	0f 87 49 01 00 00    	ja     555 <printf+0x1af>
 40c:	8b 04 85 00 10 00 00 	mov    0x1000(,%eax,4),%eax
 413:	ff e0                	jmp    *%eax
 415:	8b 45 ec             	mov    -0x14(%ebp),%eax
 418:	8d 50 04             	lea    0x4(%eax),%edx
 41b:	89 55 ec             	mov    %edx,-0x14(%ebp)
 41e:	8b 00                	mov    (%eax),%eax
 420:	99                   	cltd   
 421:	83 ec 08             	sub    $0x8,%esp
 424:	52                   	push   %edx
 425:	50                   	push   %eax
 426:	e8 0a fe ff ff       	call   235 <print_d>
 42b:	83 c4 10             	add    $0x10,%esp
 42e:	e9 42 01 00 00       	jmp    575 <printf+0x1cf>
 433:	8b 45 ec             	mov    -0x14(%ebp),%eax
 436:	8d 50 04             	lea    0x4(%eax),%edx
 439:	89 55 ec             	mov    %edx,-0x14(%ebp)
 43c:	8b 00                	mov    (%eax),%eax
 43e:	ba 00 00 00 00       	mov    $0x0,%edx
 443:	83 ec 08             	sub    $0x8,%esp
 446:	52                   	push   %edx
 447:	50                   	push   %eax
 448:	e8 50 fe ff ff       	call   29d <print_u>
 44d:	83 c4 10             	add    $0x10,%esp
 450:	e9 20 01 00 00       	jmp    575 <printf+0x1cf>
 455:	8b 45 ec             	mov    -0x14(%ebp),%eax
 458:	8d 50 04             	lea    0x4(%eax),%edx
 45b:	89 55 ec             	mov    %edx,-0x14(%ebp)
 45e:	8b 00                	mov    (%eax),%eax
 460:	83 ec 0c             	sub    $0xc,%esp
 463:	50                   	push   %eax
 464:	e8 93 fd ff ff       	call   1fc <put_string>
 469:	83 c4 10             	add    $0x10,%esp
 46c:	e9 04 01 00 00       	jmp    575 <printf+0x1cf>
 471:	8b 45 ec             	mov    -0x14(%ebp),%eax
 474:	8d 50 04             	lea    0x4(%eax),%edx
 477:	89 55 ec             	mov    %edx,-0x14(%ebp)
 47a:	8b 00                	mov    (%eax),%eax
 47c:	0f be c0             	movsbl %al,%eax
 47f:	83 ec 0c             	sub    $0xc,%esp
 482:	50                   	push   %eax
 483:	e8 8d fd ff ff       	call   215 <put_char>
 488:	83 c4 10             	add    $0x10,%esp
 48b:	e9 e5 00 00 00       	jmp    575 <printf+0x1cf>
 490:	8b 45 ec             	mov    -0x14(%ebp),%eax
 493:	8d 50 04             	lea    0x4(%eax),%edx
 496:	89 55 ec             	mov    %edx,-0x14(%ebp)
 499:	8b 00                	mov    (%eax),%eax
 49b:	83 ec 0c             	sub    $0xc,%esp
 49e:	50                   	push   %eax
 49f:	e8 32 fe ff ff       	call   2d6 <print_pointer>
 4a4:	83 c4 10             	add    $0x10,%esp
 4a7:	e9 c9 00 00 00       	jmp    575 <printf+0x1cf>
 4ac:	8b 45 ec             	mov    -0x14(%ebp),%eax
 4af:	8d 50 04             	lea    0x4(%eax),%edx
 4b2:	89 55 ec             	mov    %edx,-0x14(%ebp)
 4b5:	8b 00                	mov    (%eax),%eax
 4b7:	ba 00 00 00 00       	mov    $0x0,%edx
 4bc:	83 ec 08             	sub    $0x8,%esp
 4bf:	52                   	push   %edx
 4c0:	50                   	push   %eax
 4c1:	e8 77 fe ff ff       	call   33d <print_hex>
 4c6:	83 c4 10             	add    $0x10,%esp
 4c9:	e9 a7 00 00 00       	jmp    575 <printf+0x1cf>
 4ce:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 4d2:	8b 55 08             	mov    0x8(%ebp),%edx
 4d5:	8b 45 f4             	mov    -0xc(%ebp),%eax
 4d8:	01 d0                	add    %edx,%eax
 4da:	0f b6 00             	movzbl (%eax),%eax
 4dd:	0f be c0             	movsbl %al,%eax
 4e0:	83 f8 78             	cmp    $0x78,%eax
 4e3:	74 4b                	je     530 <printf+0x18a>
 4e5:	83 f8 78             	cmp    $0x78,%eax
 4e8:	7f 63                	jg     54d <printf+0x1a7>
 4ea:	83 f8 64             	cmp    $0x64,%eax
 4ed:	74 07                	je     4f6 <printf+0x150>
 4ef:	83 f8 75             	cmp    $0x75,%eax
 4f2:	74 1f                	je     513 <printf+0x16d>
 4f4:	eb 57                	jmp    54d <printf+0x1a7>
 4f6:	8b 45 ec             	mov    -0x14(%ebp),%eax
 4f9:	8d 50 08             	lea    0x8(%eax),%edx
 4fc:	89 55 ec             	mov    %edx,-0x14(%ebp)
 4ff:	8b 50 04             	mov    0x4(%eax),%edx
 502:	8b 00                	mov    (%eax),%eax
 504:	83 ec 08             	sub    $0x8,%esp
 507:	52                   	push   %edx
 508:	50                   	push   %eax
 509:	e8 27 fd ff ff       	call   235 <print_d>
 50e:	83 c4 10             	add    $0x10,%esp
 511:	eb 40                	jmp    553 <printf+0x1ad>
 513:	8b 45 ec             	mov    -0x14(%ebp),%eax
 516:	8d 50 08             	lea    0x8(%eax),%edx
 519:	89 55 ec             	mov    %edx,-0x14(%ebp)
 51c:	8b 50 04             	mov    0x4(%eax),%edx
 51f:	8b 00                	mov    (%eax),%eax
 521:	83 ec 08             	sub    $0x8,%esp
 524:	52                   	push   %edx
 525:	50                   	push   %eax
 526:	e8 72 fd ff ff       	call   29d <print_u>
 52b:	83 c4 10             	add    $0x10,%esp
 52e:	eb 23                	jmp    553 <printf+0x1ad>
 530:	8b 45 ec             	mov    -0x14(%ebp),%eax
 533:	8d 50 08             	lea    0x8(%eax),%edx
 536:	89 55 ec             	mov    %edx,-0x14(%ebp)
 539:	8b 50 04             	mov    0x4(%eax),%edx
 53c:	8b 00                	mov    (%eax),%eax
 53e:	83 ec 08             	sub    $0x8,%esp
 541:	52                   	push   %edx
 542:	50                   	push   %eax
 543:	e8 f5 fd ff ff       	call   33d <print_hex>
 548:	83 c4 10             	add    $0x10,%esp
 54b:	eb 06                	jmp    553 <printf+0x1ad>
 54d:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 551:	eb 22                	jmp    575 <printf+0x1cf>
 553:	eb 20                	jmp    575 <printf+0x1cf>
 555:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 559:	8b 55 08             	mov    0x8(%ebp),%edx
 55c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 55f:	01 d0                	add    %edx,%eax
 561:	0f b6 00             	movzbl (%eax),%eax
 564:	0f be c0             	movsbl %al,%eax
 567:	83 ec 0c             	sub    $0xc,%esp
 56a:	50                   	push   %eax
 56b:	e8 a5 fc ff ff       	call   215 <put_char>
 570:	83 c4 10             	add    $0x10,%esp
 573:	eb 1c                	jmp    591 <printf+0x1eb>
 575:	eb 1a                	jmp    591 <printf+0x1eb>
 577:	8b 55 08             	mov    0x8(%ebp),%edx
 57a:	8b 45 f4             	mov    -0xc(%ebp),%eax
 57d:	01 d0                	add    %edx,%eax
 57f:	0f b6 00             	movzbl (%eax),%eax
 582:	0f be c0             	movsbl %al,%eax
 585:	83 ec 0c             	sub    $0xc,%esp
 588:	50                   	push   %eax
 589:	e8 87 fc ff ff       	call   215 <put_char>
 58e:	83 c4 10             	add    $0x10,%esp
 591:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 595:	8b 45 f4             	mov    -0xc(%ebp),%eax
 598:	3b 45 f0             	cmp    -0x10(%ebp),%eax
 59b:	0f 82 2b fe ff ff    	jb     3cc <printf+0x26>
 5a1:	90                   	nop
 5a2:	90                   	nop
 5a3:	c9                   	leave  
 5a4:	c3                   	ret    

000005a5 <hello>:
 5a5:	55                   	push   %ebp
 5a6:	89 e5                	mov    %esp,%ebp
 5a8:	83 ec 28             	sub    $0x28,%esp
 5ab:	c7 45 e7 20 20 20 20 	movl   $0x20202020,-0x19(%ebp)
 5b2:	c7 45 eb 20 20 20 20 	movl   $0x20202020,-0x15(%ebp)
 5b9:	c7 45 ef 20 20 20 20 	movl   $0x20202020,-0x11(%ebp)
 5c0:	c7 45 f3 20 20 20 20 	movl   $0x20202020,-0xd(%ebp)
 5c7:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
 5cb:	83 ec 0c             	sub    $0xc,%esp
 5ce:	68 6c 10 00 00       	push   $0x106c
 5d3:	e8 ce fd ff ff       	call   3a6 <printf>
 5d8:	83 c4 10             	add    $0x10,%esp
 5db:	83 ec 08             	sub    $0x8,%esp
 5de:	8d 45 e7             	lea    -0x19(%ebp),%eax
 5e1:	50                   	push   %eax
 5e2:	68 70 10 00 00       	push   $0x1070
 5e7:	e8 ba fd ff ff       	call   3a6 <printf>
 5ec:	83 c4 10             	add    $0x10,%esp
 5ef:	83 ec 08             	sub    $0x8,%esp
 5f2:	8d 45 e7             	lea    -0x19(%ebp),%eax
 5f5:	50                   	push   %eax
 5f6:	68 a4 10 00 00       	push   $0x10a4
 5fb:	e8 a6 fd ff ff       	call   3a6 <printf>
 600:	83 c4 10             	add    $0x10,%esp
 603:	83 ec 08             	sub    $0x8,%esp
 606:	8d 45 e7             	lea    -0x19(%ebp),%eax
 609:	50                   	push   %eax
 60a:	68 a4 10 00 00       	push   $0x10a4
 60f:	e8 92 fd ff ff       	call   3a6 <printf>
 614:	83 c4 10             	add    $0x10,%esp
 617:	83 ec 08             	sub    $0x8,%esp
 61a:	8d 45 e7             	lea    -0x19(%ebp),%eax
 61d:	50                   	push   %eax
 61e:	68 d8 10 00 00       	push   $0x10d8
 623:	e8 7e fd ff ff       	call   3a6 <printf>
 628:	83 c4 10             	add    $0x10,%esp
 62b:	83 ec 08             	sub    $0x8,%esp
 62e:	8d 45 e7             	lea    -0x19(%ebp),%eax
 631:	50                   	push   %eax
 632:	68 a4 10 00 00       	push   $0x10a4
 637:	e8 6a fd ff ff       	call   3a6 <printf>
 63c:	83 c4 10             	add    $0x10,%esp
 63f:	83 ec 08             	sub    $0x8,%esp
 642:	8d 45 e7             	lea    -0x19(%ebp),%eax
 645:	50                   	push   %eax
 646:	68 a4 10 00 00       	push   $0x10a4
 64b:	e8 56 fd ff ff       	call   3a6 <printf>
 650:	83 c4 10             	add    $0x10,%esp
 653:	83 ec 08             	sub    $0x8,%esp
 656:	8d 45 e7             	lea    -0x19(%ebp),%eax
 659:	50                   	push   %eax
 65a:	68 70 10 00 00       	push   $0x1070
 65f:	e8 42 fd ff ff       	call   3a6 <printf>
 664:	83 c4 10             	add    $0x10,%esp
 667:	83 ec 0c             	sub    $0xc,%esp
 66a:	68 6c 10 00 00       	push   $0x106c
 66f:	e8 32 fd ff ff       	call   3a6 <printf>
 674:	83 c4 10             	add    $0x10,%esp
 677:	90                   	nop
 678:	c9                   	leave  
 679:	c3                   	ret    

0000067a <main>:
 67a:	8d 4c 24 04          	lea    0x4(%esp),%ecx
 67e:	83 e4 f0             	and    $0xfffffff0,%esp
 681:	ff 71 fc             	pushl  -0x4(%ecx)
 684:	55                   	push   %ebp
 685:	89 e5                	mov    %esp,%ebp
 687:	51                   	push   %ecx
 688:	83 ec 04             	sub    $0x4,%esp
 68b:	e8 b0 f9 ff ff       	call   40 <cls>
 690:	83 ec 08             	sub    $0x8,%esp
 693:	6a 0a                	push   $0xa
 695:	68 00 20 00 00       	push   $0x2000
 69a:	e8 75 f9 ff ff       	call   14 <gets>
 69f:	83 c4 10             	add    $0x10,%esp
 6a2:	eb ec                	jmp    690 <main+0x16>
 6a4:	66 90                	xchg   %ax,%ax
 6a6:	66 90                	xchg   %ax,%ax
 6a8:	66 90                	xchg   %ax,%ax
 6aa:	66 90                	xchg   %ax,%ax
 6ac:	66 90                	xchg   %ax,%ax
 6ae:	66 90                	xchg   %ax,%ax

000006b0 <__udivdi3>:
 6b0:	55                   	push   %ebp
 6b1:	89 e5                	mov    %esp,%ebp
 6b3:	57                   	push   %edi
 6b4:	56                   	push   %esi
 6b5:	53                   	push   %ebx
 6b6:	83 ec 1c             	sub    $0x1c,%esp
 6b9:	8b 45 08             	mov    0x8(%ebp),%eax
 6bc:	8b 55 14             	mov    0x14(%ebp),%edx
 6bf:	8b 75 0c             	mov    0xc(%ebp),%esi
 6c2:	8b 5d 10             	mov    0x10(%ebp),%ebx
 6c5:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 6c8:	85 d2                	test   %edx,%edx
 6ca:	75 14                	jne    6e0 <__udivdi3+0x30>
 6cc:	39 f3                	cmp    %esi,%ebx
 6ce:	76 50                	jbe    720 <__udivdi3+0x70>
 6d0:	31 ff                	xor    %edi,%edi
 6d2:	89 f2                	mov    %esi,%edx
 6d4:	f7 f3                	div    %ebx
 6d6:	89 fa                	mov    %edi,%edx
 6d8:	83 c4 1c             	add    $0x1c,%esp
 6db:	5b                   	pop    %ebx
 6dc:	5e                   	pop    %esi
 6dd:	5f                   	pop    %edi
 6de:	5d                   	pop    %ebp
 6df:	c3                   	ret    
 6e0:	39 f2                	cmp    %esi,%edx
 6e2:	76 1c                	jbe    700 <__udivdi3+0x50>
 6e4:	31 ff                	xor    %edi,%edi
 6e6:	31 c0                	xor    %eax,%eax
 6e8:	89 fa                	mov    %edi,%edx
 6ea:	83 c4 1c             	add    $0x1c,%esp
 6ed:	5b                   	pop    %ebx
 6ee:	5e                   	pop    %esi
 6ef:	5f                   	pop    %edi
 6f0:	5d                   	pop    %ebp
 6f1:	c3                   	ret    
 6f2:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 6f9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 700:	0f bd fa             	bsr    %edx,%edi
 703:	83 f7 1f             	xor    $0x1f,%edi
 706:	75 48                	jne    750 <__udivdi3+0xa0>
 708:	39 f2                	cmp    %esi,%edx
 70a:	72 07                	jb     713 <__udivdi3+0x63>
 70c:	31 c0                	xor    %eax,%eax
 70e:	3b 5d e4             	cmp    -0x1c(%ebp),%ebx
 711:	77 d5                	ja     6e8 <__udivdi3+0x38>
 713:	b8 01 00 00 00       	mov    $0x1,%eax
 718:	eb ce                	jmp    6e8 <__udivdi3+0x38>
 71a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
 720:	89 d9                	mov    %ebx,%ecx
 722:	85 db                	test   %ebx,%ebx
 724:	75 0b                	jne    731 <__udivdi3+0x81>
 726:	b8 01 00 00 00       	mov    $0x1,%eax
 72b:	31 d2                	xor    %edx,%edx
 72d:	f7 f3                	div    %ebx
 72f:	89 c1                	mov    %eax,%ecx
 731:	31 d2                	xor    %edx,%edx
 733:	89 f0                	mov    %esi,%eax
 735:	f7 f1                	div    %ecx
 737:	89 c6                	mov    %eax,%esi
 739:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 73c:	89 f7                	mov    %esi,%edi
 73e:	f7 f1                	div    %ecx
 740:	89 fa                	mov    %edi,%edx
 742:	83 c4 1c             	add    $0x1c,%esp
 745:	5b                   	pop    %ebx
 746:	5e                   	pop    %esi
 747:	5f                   	pop    %edi
 748:	5d                   	pop    %ebp
 749:	c3                   	ret    
 74a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
 750:	89 f9                	mov    %edi,%ecx
 752:	b8 20 00 00 00       	mov    $0x20,%eax
 757:	29 f8                	sub    %edi,%eax
 759:	d3 e2                	shl    %cl,%edx
 75b:	89 55 e0             	mov    %edx,-0x20(%ebp)
 75e:	89 c1                	mov    %eax,%ecx
 760:	89 da                	mov    %ebx,%edx
 762:	d3 ea                	shr    %cl,%edx
 764:	8b 4d e0             	mov    -0x20(%ebp),%ecx
 767:	09 d1                	or     %edx,%ecx
 769:	89 f2                	mov    %esi,%edx
 76b:	89 4d e0             	mov    %ecx,-0x20(%ebp)
 76e:	89 f9                	mov    %edi,%ecx
 770:	d3 e3                	shl    %cl,%ebx
 772:	89 c1                	mov    %eax,%ecx
 774:	89 5d dc             	mov    %ebx,-0x24(%ebp)
 777:	d3 ea                	shr    %cl,%edx
 779:	8b 5d e4             	mov    -0x1c(%ebp),%ebx
 77c:	89 f9                	mov    %edi,%ecx
 77e:	d3 e6                	shl    %cl,%esi
 780:	89 c1                	mov    %eax,%ecx
 782:	d3 eb                	shr    %cl,%ebx
 784:	09 de                	or     %ebx,%esi
 786:	89 f0                	mov    %esi,%eax
 788:	f7 75 e0             	divl   -0x20(%ebp)
 78b:	89 d6                	mov    %edx,%esi
 78d:	89 c3                	mov    %eax,%ebx
 78f:	f7 65 dc             	mull   -0x24(%ebp)
 792:	89 55 e0             	mov    %edx,-0x20(%ebp)
 795:	39 d6                	cmp    %edx,%esi
 797:	72 27                	jb     7c0 <__udivdi3+0x110>
 799:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 79c:	89 f9                	mov    %edi,%ecx
 79e:	d3 e2                	shl    %cl,%edx
 7a0:	39 c2                	cmp    %eax,%edx
 7a2:	73 05                	jae    7a9 <__udivdi3+0xf9>
 7a4:	3b 75 e0             	cmp    -0x20(%ebp),%esi
 7a7:	74 17                	je     7c0 <__udivdi3+0x110>
 7a9:	89 d8                	mov    %ebx,%eax
 7ab:	31 ff                	xor    %edi,%edi
 7ad:	e9 36 ff ff ff       	jmp    6e8 <__udivdi3+0x38>
 7b2:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 7b9:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 7c0:	8d 43 ff             	lea    -0x1(%ebx),%eax
 7c3:	31 ff                	xor    %edi,%edi
 7c5:	e9 1e ff ff ff       	jmp    6e8 <__udivdi3+0x38>
 7ca:	66 90                	xchg   %ax,%ax
 7cc:	66 90                	xchg   %ax,%ax
 7ce:	66 90                	xchg   %ax,%ax

000007d0 <__umoddi3>:
 7d0:	55                   	push   %ebp
 7d1:	89 e5                	mov    %esp,%ebp
 7d3:	57                   	push   %edi
 7d4:	56                   	push   %esi
 7d5:	53                   	push   %ebx
 7d6:	83 ec 2c             	sub    $0x2c,%esp
 7d9:	8b 55 14             	mov    0x14(%ebp),%edx
 7dc:	8b 75 08             	mov    0x8(%ebp),%esi
 7df:	8b 5d 0c             	mov    0xc(%ebp),%ebx
 7e2:	8b 7d 10             	mov    0x10(%ebp),%edi
 7e5:	85 d2                	test   %edx,%edx
 7e7:	75 27                	jne    810 <__umoddi3+0x40>
 7e9:	39 df                	cmp    %ebx,%edi
 7eb:	76 43                	jbe    830 <__umoddi3+0x60>
 7ed:	89 f0                	mov    %esi,%eax
 7ef:	89 da                	mov    %ebx,%edx
 7f1:	f7 f7                	div    %edi
 7f3:	89 d6                	mov    %edx,%esi
 7f5:	31 d2                	xor    %edx,%edx
 7f7:	89 f0                	mov    %esi,%eax
 7f9:	83 c4 2c             	add    $0x2c,%esp
 7fc:	5b                   	pop    %ebx
 7fd:	5e                   	pop    %esi
 7fe:	5f                   	pop    %edi
 7ff:	5d                   	pop    %ebp
 800:	c3                   	ret    
 801:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 808:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 80f:	90                   	nop
 810:	89 75 e0             	mov    %esi,-0x20(%ebp)
 813:	39 da                	cmp    %ebx,%edx
 815:	76 49                	jbe    860 <__umoddi3+0x90>
 817:	89 da                	mov    %ebx,%edx
 819:	89 f0                	mov    %esi,%eax
 81b:	83 c4 2c             	add    $0x2c,%esp
 81e:	5b                   	pop    %ebx
 81f:	5e                   	pop    %esi
 820:	5f                   	pop    %edi
 821:	5d                   	pop    %ebp
 822:	c3                   	ret    
 823:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 82a:	8d b6 00 00 00 00    	lea    0x0(%esi),%esi
 830:	89 f9                	mov    %edi,%ecx
 832:	85 ff                	test   %edi,%edi
 834:	75 0b                	jne    841 <__umoddi3+0x71>
 836:	b8 01 00 00 00       	mov    $0x1,%eax
 83b:	31 d2                	xor    %edx,%edx
 83d:	f7 f7                	div    %edi
 83f:	89 c1                	mov    %eax,%ecx
 841:	89 d8                	mov    %ebx,%eax
 843:	31 d2                	xor    %edx,%edx
 845:	f7 f1                	div    %ecx
 847:	89 f0                	mov    %esi,%eax
 849:	f7 f1                	div    %ecx
 84b:	89 d6                	mov    %edx,%esi
 84d:	31 d2                	xor    %edx,%edx
 84f:	eb a6                	jmp    7f7 <__umoddi3+0x27>
 851:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 858:	8d b4 26 00 00 00 00 	lea    0x0(%esi,%eiz,1),%esi
 85f:	90                   	nop
 860:	0f bd ca             	bsr    %edx,%ecx
 863:	83 f1 1f             	xor    $0x1f,%ecx
 866:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 869:	75 25                	jne    890 <__umoddi3+0xc0>
 86b:	39 da                	cmp    %ebx,%edx
 86d:	0f 82 ad 00 00 00    	jb     920 <__umoddi3+0x150>
 873:	89 d8                	mov    %ebx,%eax
 875:	39 f7                	cmp    %esi,%edi
 877:	0f 86 a3 00 00 00    	jbe    920 <__umoddi3+0x150>
 87d:	8b 75 e0             	mov    -0x20(%ebp),%esi
 880:	89 c2                	mov    %eax,%edx
 882:	89 f0                	mov    %esi,%eax
 884:	83 c4 2c             	add    $0x2c,%esp
 887:	5b                   	pop    %ebx
 888:	5e                   	pop    %esi
 889:	5f                   	pop    %edi
 88a:	5d                   	pop    %ebp
 88b:	c3                   	ret    
 88c:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
 890:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
 893:	b8 20 00 00 00       	mov    $0x20,%eax
 898:	29 c8                	sub    %ecx,%eax
 89a:	d3 e2                	shl    %cl,%edx
 89c:	89 55 dc             	mov    %edx,-0x24(%ebp)
 89f:	89 c1                	mov    %eax,%ecx
 8a1:	89 fa                	mov    %edi,%edx
 8a3:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8a6:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8a9:	d3 ea                	shr    %cl,%edx
 8ab:	09 d0                	or     %edx,%eax
 8ad:	8b 55 e0             	mov    -0x20(%ebp),%edx
 8b0:	89 45 dc             	mov    %eax,-0x24(%ebp)
 8b3:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8b6:	89 c1                	mov    %eax,%ecx
 8b8:	d3 e7                	shl    %cl,%edi
 8ba:	89 d1                	mov    %edx,%ecx
 8bc:	89 7d d8             	mov    %edi,-0x28(%ebp)
 8bf:	89 df                	mov    %ebx,%edi
 8c1:	d3 ef                	shr    %cl,%edi
 8c3:	89 c1                	mov    %eax,%ecx
 8c5:	89 f0                	mov    %esi,%eax
 8c7:	d3 e3                	shl    %cl,%ebx
 8c9:	89 d1                	mov    %edx,%ecx
 8cb:	89 fa                	mov    %edi,%edx
 8cd:	d3 e8                	shr    %cl,%eax
 8cf:	0f b6 4d e4          	movzbl -0x1c(%ebp),%ecx
 8d3:	09 d8                	or     %ebx,%eax
 8d5:	d3 e6                	shl    %cl,%esi
 8d7:	f7 75 dc             	divl   -0x24(%ebp)
 8da:	89 d3                	mov    %edx,%ebx
 8dc:	89 75 d4             	mov    %esi,-0x2c(%ebp)
 8df:	89 f1                	mov    %esi,%ecx
 8e1:	f7 65 d8             	mull   -0x28(%ebp)
 8e4:	89 c6                	mov    %eax,%esi
 8e6:	89 d7                	mov    %edx,%edi
 8e8:	39 d3                	cmp    %edx,%ebx
 8ea:	72 06                	jb     8f2 <__umoddi3+0x122>
 8ec:	75 0e                	jne    8fc <__umoddi3+0x12c>
 8ee:	39 c1                	cmp    %eax,%ecx
 8f0:	73 0a                	jae    8fc <__umoddi3+0x12c>
 8f2:	2b 45 d8             	sub    -0x28(%ebp),%eax
 8f5:	1b 55 dc             	sbb    -0x24(%ebp),%edx
 8f8:	89 d7                	mov    %edx,%edi
 8fa:	89 c6                	mov    %eax,%esi
 8fc:	0f b6 4d e0          	movzbl -0x20(%ebp),%ecx
 900:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 903:	29 f0                	sub    %esi,%eax
 905:	19 fb                	sbb    %edi,%ebx
 907:	8b 7d e4             	mov    -0x1c(%ebp),%edi
 90a:	89 de                	mov    %ebx,%esi
 90c:	d3 e6                	shl    %cl,%esi
 90e:	89 f9                	mov    %edi,%ecx
 910:	d3 e8                	shr    %cl,%eax
 912:	d3 eb                	shr    %cl,%ebx
 914:	09 c6                	or     %eax,%esi
 916:	e9 fc fe ff ff       	jmp    817 <__umoddi3+0x47>
 91b:	8d 74 26 00          	lea    0x0(%esi,%eiz,1),%esi
 91f:	90                   	nop
 920:	29 fe                	sub    %edi,%esi
 922:	19 d3                	sbb    %edx,%ebx
 924:	89 75 e0             	mov    %esi,-0x20(%ebp)
 927:	89 d8                	mov    %ebx,%eax
 929:	e9 4f ff ff ff       	jmp    87d <__umoddi3+0xad>
