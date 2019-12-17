/* The bootloader will look at this image and start execution at the symbol
   designated as the entry point. */
ENTRY(amain)
 
/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
	. = 0x40000000;	/* program zaczyna się od pierwszego gigabajta */

	.text ALIGN(0x1000) :
	{
		*(.text)
	}
 
	.rodata ALIGN(0x1000) :
	{
		*(.rodata*)
	}
 
	.data ALIGN(0x1000) :
	{
		*(.data)
	}
 
	.bss ALIGN(0x1000) :
	{
		*(COMMON)
		*(.bss)
	}
}