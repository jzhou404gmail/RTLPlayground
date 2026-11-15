#!/usr/bin/env bash
# Guard the local fork against losing work while syncing from upstream.
#
#   tools/sync-guard.sh pre [--strict] [--patches] [--bundle]
#   tools/sync-guard.sh post [--quick]
#   tools/sync-guard.sh status
#
# "pre" must run before a merge/rebase from origin/main. It refuses a dirty
# worktree, prints the local commits, and creates a local backup tag at HEAD.
# With --strict it instead refuses to continue while local commits are ahead
# of the base, which is the literal "no local commits" gate. The default backup is a tag at HEAD;
# --patches additionally writes format-patch/diff files, --bundle writes a full
# git bundle (large). Push the branch to fork/main for an off-machine backup.
#
# "post" runs after the sync and checks that the local features this fork
# carries are still present, then runs the host test suite. A sync that drops
# the logout button or the passwd_hash flow fails here instead of shipping.
set -euo pipefail

ROOT=$(git rev-parse --show-toplevel)
cd "$ROOT"

case "${1:-}" in
-h|--help)
	sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
	exit 0
	;;
esac

MODE=${1:-status}
shift || true

QUICK=0
STRICT=0
PATCHES=0
BUNDLE=0
for arg in "$@"; do
	case "$arg" in
	--quick) QUICK=1 ;;
	--strict) STRICT=1 ;;
	--patches) PATCHES=1 ;;
	--bundle) BUNDLE=1 ;;
	-h|--help)
		sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
		exit 0
		;;
	*) echo "[fail] unknown option: $arg" >&2; exit 2 ;;
	esac
done

BASE=${BASE:-origin/main}
FORK=${FORK:-fork/main}
BRANCH=$(git rev-parse --abbrev-ref HEAD)
HEAD=$(git rev-parse --short HEAD)

fail() { echo "[fail] $*" >&2; exit 1; }
warn() { echo "[warn] $*"; }
ok()   { echo "[ok]   $*"; }

git rev-parse --verify "$BASE^{commit}" >/dev/null 2>&1 \
	|| fail "missing $BASE; run: git fetch origin"

case "$MODE" in
status)
	echo "branch:  $BRANCH"
	echo "head:    $HEAD $(git log -1 --pretty=%s)"
	echo "base:    $BASE ($(git rev-parse --short "$BASE"))"
	echo "ahead:   $(git rev-list --count "$BASE"..HEAD) commit(s)"
	git log --oneline -10 "$BASE"..HEAD | sed 's/^/         /'
	[ "$(git rev-list --count "$BASE"..HEAD)" -gt 10 ] && echo "         ..."
	echo "backups: $(git tag -l 'pre-sync-*' | tail -5 | tr '\n' ' ')"
	;;

pre)
	DIRTY=$(git status --porcelain --untracked-files=no)
	[ -z "$DIRTY" ] || fail "tracked changes present; commit or stash first:
$DIRTY"

	UNTRACKED=$(git ls-files --others --exclude-standard)
	[ -z "$UNTRACKED" ] || warn "untracked, non-ignored files are not backed up:
$UNTRACKED"

	AHEAD=$(git rev-list --count "$BASE"..HEAD)
	echo "branch=$BRANCH head=$HEAD base=$BASE local-commits=$AHEAD"

	if [ "$AHEAD" -eq 0 ]; then
		ok "no local commits ahead of $BASE"
		exit 0
	fi

	git log --oneline -10 "$BASE"..HEAD | sed 's/^/       /'
	[ "$AHEAD" -gt 10 ] && echo "       ... ($AHEAD total)"

	if [ "$STRICT" -eq 1 ]; then
		fail "strict: $AHEAD local commit(s) ahead of $BASE; push or merge them first"
	fi

	TS=$(date +%Y%m%d-%H%M%S)
	TAG="pre-sync-$TS"
	while git rev-parse --verify "refs/tags/$TAG" >/dev/null 2>&1; do
		TS="$TS-$$"
		TAG="pre-sync-$TS"
	done
	git tag -a "$TAG" -m "pre-upstream-sync backup of $BRANCH at $HEAD" HEAD

	if [ "$PATCHES" -eq 1 ] || [ "$BUNDLE" -eq 1 ]; then
		DIR=".local-records/pre-sync-$TS"
		mkdir -p "$DIR"
		printf '%s\n' "$HEAD" > "$DIR/HEAD"
		printf '%s\n' "$BASE" > "$DIR/BASE"
		[ "$PATCHES" -eq 1 ] && git format-patch -q -o "$DIR" "$BASE"..HEAD >/dev/null
		[ "$PATCHES" -eq 1 ] && git diff --binary "$BASE"..HEAD > "$DIR/local.diff"
		[ "$BUNDLE" -eq 1 ] && git bundle create "$DIR/repo.bundle" --branches --tags >/dev/null
		ok "backup files: $DIR"
	fi
	ok "backup tag: $TAG"
	if git rev-parse --verify "$FORK^{commit}" >/dev/null 2>&1 \
		&& [ -z "$(git rev-list "$FORK"..HEAD)" ]; then
		ok "HEAD is already on $FORK"
	else
		warn "HEAD is not on $FORK; push it for an off-machine backup"
	fi
	;;

post)
	FAIL=0
	check_file() { # <file> <literal> <description>
		if [ -f "$1" ] && grep -Fq -- "$2" "$1"; then
			ok "$3"
		else
			echo "[fail] $3 ($1)" >&2
			FAIL=$((FAIL + 1))
		fi
	}
	check_test() { # <description> <shell condition>
		if eval "$2"; then ok "$1"; else echo "[fail] $1" >&2; FAIL=$((FAIL + 1)); fi
	}

	check_file html/index.html 'id="logoutBtn"' "header has the logout button"
	check_file html/app.js 'hdr_logout' "logout i18n strings exist"
	check_file html/app.js '$("logoutBtn")' "logout button has a handler"
	check_file html/app.js 'crypto.getRandomValues(salt)' "password change rotates the salt"
	check_file html/app.js 'sy_pw_ok' "password-change success message exists"
	check_file html/app.js 'passwd_hash' "config editor recognises passwd_hash"
	check_file html/app.js 'vlan\s+switch' "config editor recognises vlan switch"
	check_file html/login.html 'pw2row' "first-run password setup page exists"
	check_file html/login.html 'legacy' "legacy plaintext login path exists"
	check_file httpd/httpd.c 'setup_body_take' "first-run setup handler exists"
	check_file httpd/httpd.c 'send_server_error' "auth write failure is reported"
	check_file auth.c 'auth_salt_tmp' "failed digest parse cannot corrupt the live hash"
	check_file auth.c 'legacy' "auth.json still reports the legacy flag"
	check_file auth.c 'No terminator' "config write-back guard exists"
	check_file rtlplayground.c 'auth_persist' "serial auth writes are persisted"
	check_file tools/passwd_hash.py 'passwd_hash' "host passwd_hash tool exists"
	check_test "test_auth.c exists" '[ -f test/test_auth.c ]'
	check_file test/test_httpd_tx.c 'scenario_setup' "HTTP auth scenarios exist"
	check_test "passwd_hash tool is executable" '[ -x tools/passwd_hash.py ]'
	check_test "no whitespace errors" 'git diff --check'

	if command -v node >/dev/null 2>&1; then
		if node --check html/app.js >/dev/null 2>&1; then
			ok "app.js parses"
		else
			echo "[fail] app.js does not parse" >&2
			FAIL=$((FAIL + 1))
		fi
	fi

	if [ "$QUICK" -eq 0 ]; then
		if make -C test >/tmp/sync-guard-test.log 2>&1; then
			ok "host test suite"
		else
			echo "[fail] host test suite; see /tmp/sync-guard-test.log" >&2
			FAIL=$((FAIL + 1))
		fi
	fi

	if [ "$FAIL" -ne 0 ]; then
		echo "[fail] $FAIL check(s) failed; do not commit the sync" >&2
		exit 1
	fi
	ok "post-sync guard: all local features and tests are present"
	;;

*)
	echo "usage: $0 {pre [--strict] [--patches] [--bundle]|post [--quick]|status}" >&2
	exit 2
	;;
esac
