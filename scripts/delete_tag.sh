#!/bin/sh
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 TAG_NAME" >&2
  exit 1
fi

TAG_NAME=$1

echo -n "Are you sure to delete tag:${TAG_NAME} (Y/N)? "
while read -r -n 1 -s answer; do
  if [[ $answer = [YyNn] ]]; then
    [[ $answer = [Yy] ]] && retval=0
    [[ $answer = [Nn] ]] && retval=1
    break
  fi
done

if [ $retval -eq 1 ]; then
  exit 1
fi

echo ""
echo  "Start to delete tag:${TAG_NAME}."

git tag -d ${TAG_NAME}
if [ $? -ne 0 ]; then
  echo "Delete local tag:${TAG_NAME} failed!" >&2
  exit 1
fi
git push --delete origin ${TAG_NAME}
if [ $? -ne 0 ]; then
  echo "Delete remote tag:${TAG_NAME} failed!" >&2
  exit 1
fi
echo  "Success to delete tag:${TAG_NAME}!"