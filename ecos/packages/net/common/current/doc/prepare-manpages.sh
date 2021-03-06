#! /bin/sh

echo 'dude, assuming you are running this script from the ecos-docs/tcpip directory'

MANPAGE_LIST=`find manpages -type f -name '*.?'`

echo "MANPAGE_LIST is $MANPAGE_LIST"

echo 'removing the previous file tcpip-manpages.sgml'
/bin/rm -f tcpip-manpages.sgml
touch tcpip-manpages.sgml
echo '<!-- HEY YOU!!!!!!!!! -->' >> tcpip-manpages.sgml
echo '<!-- this file is automatically generated by the script -->' >> tcpip-manpages.sgml
echo '<!-- ' "     $0    " ' -->' >> tcpip-manpages.sgml
echo '<!-- so PLEASE do not modify it: your changes will be lost -->' >> tcpip-manpages.sgml
echo >> tcpip-manpages.sgml
echo >> tcpip-manpages.sgml

echo "<chapter id=\"tcpip-library-reference\">" >> tcpip-manpages.sgml
echo "  <title>TCP/IP Library Reference</title>" >> tcpip-manpages.sgml

echo >> tcpip-manpages.sgml
echo >> tcpip-manpages.sgml

for manpage in $MANPAGE_LIST
do
    echo "processing $manpage"
    # get the title for this section
    manpage_title=`egrep '^\.Dt' $manpage | awk '{print $2}' | tr 'A-Z' 'a-z'`
    # note that _ is illegal in an id, so we canonicalize it to -
    docbook_section_id=`echo $manpage_title | sed 's/_/-/g'`
    # now prepare out a section and title
    echo "  <sect1 id=\"net-common-tcpip-manpages-$docbook_section_id\">" >> tcpip-manpages.sgml
    echo "    <title>$manpage_title</title>" >> tcpip-manpages.sgml
    # we make it <screen> so that it is a monospaced font
    echo "    <screen>" >> tcpip-manpages.sgml

    # now put the contents into this section
    cat $manpage | groff -Tascii -mandoc | sed 's/\_\(.\)/\1/g' \
      | sed 's/\(.\)\(.\)/\1/g' \
      | sed 's/\&/\&amp;/g' \
      | sed 's/</\&lt;/g' \
      | sed 's/+o/o/g' >> tcpip-manpages.sgml

    # now close out the section
    echo "    </screen>" >> tcpip-manpages.sgml
    echo "  </sect1>" >> tcpip-manpages.sgml
    echo >> tcpip-manpages.sgml
done

echo >> tcpip-manpages.sgml
echo "</chapter>" >> tcpip-manpages.sgml

cat <<EOF >> tcpip-manpages.sgml

<!-- Keep this comment at the end of the file
Local variables:
mode: sgml
sgml-omittag:nil
sgml-shorttag:t
sgml-namecase-general:t
sgml-general-insert-case:lower
sgml-minimize-attributes:nil
sgml-always-quote-attributes:t
sgml-indent-step:2
sgml-indent-data:t
sgml-parent-document:("tcpip.sgml" "book" "chapter")
sgml-exposed-tags:nil
sgml-local-catalogs:nil
sgml-local-ecat-files:nil
sgml-doctype:"book"
End:
-->

EOF
