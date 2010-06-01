textures/sfx/largerblock3b3_pent
//last edited by **HD
{
	qer_editorimage textures/sfx/largerblock3b3_pent.tga
	{
		map textures/sfx/largerblock3b3_pent.tga
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendfunc filter
	}
	{
		map textures/sfx/pentagramfloor_red_glow.tga
		blendfunc add
		rgbGen wave sin 1 .1 0.25 1
		depthfunc equal
  }
}