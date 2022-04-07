echo "start install patch!"
 
payload_offset=$(($(grep -na -m1 "^PAYLOAD:$" $0|cut -d':' -f1) + 1))
tail -n +$payload_offset $0 | tar zx -C / > /dev/null 2>&1
 
exit 0
