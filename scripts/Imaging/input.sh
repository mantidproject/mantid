read -p "REMOVE CREATED DIRECTORIES FROM ~/temp/? ONLY CLICK YES IF ALL PASSED SUCCESSFULLY! [yes, N]: " -r
echo    # (optional) move to a new line
if [[ $REPLY =~ (yes)|(YES)|(Yes) ]]
then
echo Hi
fi