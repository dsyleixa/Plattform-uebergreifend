# qtsaveas.sh
# qt5 creator: save project as...
# start from current project folder!

read -p "Enter new project name: " xxxx   # ask for new project name
echo "your new project name: $xxxx"       # echo entered name

mkdir ../$xxxx/     # make new destination dir by entered name
cp *.* ../$xxxx/    # copy all source files to new destination dir 

rm ../$xxxx/*.pro.user                 # delete .pro.user file in destination
mv ../$xxxx/*.pro  ../$xxxx/$xxxx.pro  # rename old .pro project file name by new name 