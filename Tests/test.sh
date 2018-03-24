$1
result=$?
if [ $result = $2 ]; then
	echo "PASS"
else
	echo "FAIL: expected $2 but got $result from $1"
fi
