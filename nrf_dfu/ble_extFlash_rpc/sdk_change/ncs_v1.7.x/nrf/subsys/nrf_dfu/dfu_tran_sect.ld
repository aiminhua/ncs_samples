SECTION_DATA_PROLOGUE(dfu_trans_sections,,)
{
	__start_dfu_trans = .;
	KEEP(*(SORT(.dfu_trans*)));
	__stop_dfu_trans = .;
} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)	
