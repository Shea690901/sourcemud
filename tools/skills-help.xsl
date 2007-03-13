<?xml version="1.0"?>
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  <xsl:template match="/skills">

<!-- header -->
<xsl:text><![CDATA[
# Generated by skills-help.xsl
# DO NOT EDIT
]]></xsl:text>

<!-- help sections -->
<xsl:for-each select="skill">
<xsl:text>@begin </xsl:text><xsl:value-of select="@name" /><xsl:text>
  </xsl:text><xsl:value-of select="help" /><xsl:text>
@end
</xsl:text>
</xsl:for-each>

  </xsl:template>

  <xsl:template name="skill" mode="id">
  </xsl:template>
</xsl:stylesheet>