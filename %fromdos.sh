:
find . -type f  ! -name \*.exe ! -name \*.doc ! -name \*.dat ! -name \*.pgp \
	! -name \*.lib ! -name \*.tgz |\
xargs fromdos

tar xzf python.tgz
make rename
(cd scrlib && make rename)
(cd pgplib && make rename)
