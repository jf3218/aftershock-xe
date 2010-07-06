spark
{
	cull none
	entityMergable
	{
		map sprites/spark.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}

plasmaSpark
{
	cull none
	entityMergable
	{
		map sprites/plasmaSpark.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		entity
		alphaGen	vertex
	}
}