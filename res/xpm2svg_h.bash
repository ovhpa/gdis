#!/bin/bash
# generate a header with all icons in svg embedded format from individual xpm files
# 1. convert all xpm files to embedded SVG
XPM_FILES=`ls -1f *.xpm`
for this_xpm in ${XPM_FILES}
do
  echo "Processing: $this_xpm ..."
  this_name=`basename $this_xpm .xpm`
  this_var=`echo "DATA_$this_name" | sed 's/./\U&/g'`
  convert $this_xpm $this_name.png
  vtracer -f 0 --mode pixel --input $this_name.png --output $this_name.svg
  echo "const char* const $this_var = R\"svg(" > $this_name.h && sed -n '/<svg/,/<\/svg/p' $this_name.svg >> $this_name.h && echo ")svg\";" >> $this_name.h
  echo "... Done!"
done
echo "Creating header..."
# 2. create the header (c extension for now)
echo "#ifndef  ALL_ICONS_H" > all_icons.c
echo "#define  ALL_ICONS_H" >> all_icons.c
cat *.h >> all_icons.c
echo "#endif //ALL_ICONS_H" >> all_icons.c
# 3. cleanup intermediate files
rm *.h
rm *.svg
rm *.png
# 4. final header result
mv all_icons.c all_icons.h
echo "... All done!"
# note: the icon_entry registry is obtain using:
# grep "const char\* const" gui/all_icons.h | awk '{printf $4 "\n"}' | sed -e 's/DATA_\(.*\)/{.data = DATA_\1, .name = "\1"},/g'
