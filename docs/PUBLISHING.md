# Publishing SHEETZ! to GitHub

This project is already laid out as a repository: it has source, CMake configuration,
a CI workflow, an AGPL licence and a README. The only missing piece is the remote
repository. Pick one of the routes below.

> Do not let GitHub add a README, `.gitignore` or licence when you create the empty
> repository. Those files already exist here, and an extra initial commit only makes the
> first push harder.

---

## Route 1: Git on the command line (recommended)

Install Git for Windows if needed:

```powershell
winget install --id Git.Git
```

Open a new terminal, then from this folder:

```powershell
git init -b main
git add .
git commit -m "Initial SHEETZ! implementation"
```

Create an empty repository named `SHEETZ` at <https://github.com/new>, then connect it:

```powershell
git remote add origin https://github.com/YOUR-GITHUB-USERNAME/SHEETZ.git
git push -u origin main
```

If Git asks you to sign in, use a browser sign-in or a personal access token as the
password. GitHub no longer accepts an account password for HTTPS pushes.

### GitHub CLI shortcut

With the GitHub CLI installed and signed in, the empty repository and the push can be
created in one step:

```powershell
winget install --id GitHub.cli
gh auth login
gh repo create SHEETZ --public --source=. --remote=origin --push
```

Use `--private` instead of `--public` if you do not want the source visible yet.

---

## Route 2: Browser-only upload

1. Create an empty repository named `SHEETZ` at <https://github.com/new>.
2. On the empty repository page choose **uploading an existing file**.
3. Drag the **contents** of this folder into the upload area, not the folder itself:
   - `CMakeLists.txt`
   - `CMakePresets.json`
   - `README.md`
   - `LICENSE`
   - `.gitignore`
   - the `Source` folder
   - the `docs` folder
   - the `.github` folder
4. Commit the files.

After the upload, check that `.github/workflows/build.yml` is present in the repository.
If your browser hid dot-files or the workflow is missing, install GitHub Desktop or Git
and use Route 1 instead; without that file GitHub Actions will not build the plugin.

---

## Route 3: GitHub Desktop

1. Install GitHub Desktop and sign in.
2. Choose **File > Add local repository** and select this folder.
3. If it offers to create a repository, accept, and keep the default branch name `main`.
4. Click **Publish repository** and clear **Keep this code private** if you want a public
   AGPL project.

---

## After the first push

1. Open the **Actions** tab. The **Build** workflow starts automatically.
2. Wait for the Windows, macOS and Linux jobs to finish. The first run downloads JUCE and
   takes noticeably longer than later runs.
3. Open a successful run and download the artifact for your platform:
   - `SHEETZ-windows-x64`
   - `SHEETZ-macos-universal`
   - `SHEETZ-linux-x64`
4. Follow [Install a build](../README.md#install-a-build) in the README.

To publish a permanent download, create a release and attach the zipped artifacts:

```powershell
gh release create v1.0.0 --title "SHEETZ! 1.0.0" --notes "First public build."
```

If you change the default branch from `main` to `master`, either rename it or add the
branch to the `push.branches` list in `.github/workflows/build.yml`.
