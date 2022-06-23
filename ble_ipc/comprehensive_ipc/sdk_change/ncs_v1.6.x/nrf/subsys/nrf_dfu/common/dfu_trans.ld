	SECTION_DATA_PROLOGUE(.dfu_trans,,)
	{
		__start_dfu_trans = .;
		KEEP(*(SORT(.dfu_trans*)));
		__stop_dfu_trans = .;
	} GROUP_LINK_IN(ROMABLE_REGION)
