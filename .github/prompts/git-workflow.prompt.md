# Git Workflow & Project Management Prompt

## Context
You are managing the GPC (GPS Console) project with a clean git workflow and structured development process.

## Git Workflow

### Commit Message Standards
Follow conventional commit format:
```
feat: Add new GPS feature
fix: Resolve connection issue  
docs: Update README with new features
refactor: Improve code organization
chore: Update dependencies
test: Add GPS hardware tests
```

### Commit Categories
- **feat**: New features
- **fix**: Bug fixes
- **docs**: Documentation updates
- **refactor**: Code improvements without new features
- **chore**: Maintenance tasks
- **test**: Testing additions/improvements

### Branch Strategy
- **master**: Main development branch (stable)
- **feature/xxx**: Feature development branches
- **hotfix/xxx**: Critical bug fixes
- **release/xxx**: Release preparation

## Project Organization

### Documentation Updates
Always update these files when making changes:
- **README.md**: Feature lists, installation, usage
- **NOTE.md**: Development notes and decisions
- **.vscode/copilot-instructions.md**: Project context

### File Organization
```
gpc/
├── src/                    # Source code
├── data/                   # GPS data files (git ignored)
├── .vscode/               # Development tools
│   ├── copilot-instructions.md
│   └── prompts/           # Context prompts
├── .gitignore             # Excluded files
├── Makefile               # Build configuration
├── README.md              # Project documentation
└── NOTE.md                # Development notes
```

## Development Workflow

### New Feature Development
1. **Create Feature Branch**
   ```bash
   git checkout -b feature/new-gps-feature
   ```

2. **Develop with Incremental Commits**
   ```bash
   git add .
   git commit -m "feat: Initial GPS feature implementation"
   ```

3. **Test Thoroughly**
   - Build and test functionality
   - Test with real GPS hardware
   - Verify no regressions

4. **Update Documentation**
   - Update README.md feature list
   - Add to completed features section
   - Update usage instructions if needed

5. **Merge to Master**
   ```bash
   git checkout master
   git merge feature/new-gps-feature
   git push origin master
   ```

### Bug Fix Workflow
1. **Identify Issue**
   - Reproduce bug reliably
   - Document steps to reproduce
   - Identify affected components

2. **Create Fix**
   ```bash
   git checkout -b fix/connection-issue
   ```

3. **Implement & Test**
   - Make minimal necessary changes
   - Test fix thoroughly
   - Verify no side effects

4. **Commit & Merge**
   ```bash
   git commit -m "fix: Resolve GPS connection timeout issue"
   git checkout master
   git merge fix/connection-issue
   ```

## Project Management

### Feature Tracking
Use README.md sections:
- **✅ Tamamlanan Özellikler**: Completed features
- **🔄 Kısa Vadeli**: Short-term development
- **📋 Orta Vadeli**: Medium-term goals
- **🚀 Uzun Vadeli**: Long-term vision

### Development Priorities
1. **Core Functionality**: GPS connectivity, data parsing
2. **User Experience**: UI improvements, error handling
3. **Advanced Features**: Track analysis, waypoints
4. **Performance**: Optimization, memory usage
5. **Platform Support**: Linux compatibility, packaging

### Code Quality Gates
Before committing:
- [ ] Code compiles without warnings
- [ ] Functionality tested with GPS hardware
- [ ] No memory leaks detected
- [ ] Documentation updated if needed
- [ ] Commit message follows conventions

## Release Management

### Version Strategy
- **Major**: 1.0, 2.0 - Significant architecture changes
- **Minor**: 1.1, 1.2 - New features, substantial improvements
- **Patch**: 1.1.1, 1.1.2 - Bug fixes, small improvements

### Release Process
1. **Feature Freeze**: Complete all planned features
2. **Testing Phase**: Comprehensive testing with GPS hardware
3. **Documentation**: Update all documentation
4. **Tag Release**: `git tag v1.0.0`
5. **Release Notes**: Summarize changes and improvements

### Backup & Archive
- **Git Remote**: Ensure all changes pushed to remote
- **Data Backup**: GPS logs and tracks not in git
- **Documentation**: Keep offline copies of important docs

## Collaboration Guidelines

### Code Review
- **Self Review**: Always review your own changes before commit
- **Testing**: Test with real GPS hardware when possible
- **Documentation**: Update docs for user-facing changes
- **Standards**: Follow established coding standards

### Issue Management
- **Bug Reports**: Include steps to reproduce, GPS hardware used
- **Feature Requests**: Describe use case and expected behavior
- **Enhancement**: Propose improvements with clear benefits

### Knowledge Sharing
- **Code Comments**: Document complex GPS calculations
- **Commit Messages**: Explain why changes were made
- **README Updates**: Keep user documentation current
- **Prompt Files**: Maintain context for future development

## Common Git Commands

### Daily Workflow
```bash
# Check status
git status

# Stage and commit changes
git add .
git commit -m "feat: Add satellite tracking improvements"

# Push to remote
git push origin master

# Pull latest changes
git pull origin master
```

### Branch Management
```bash
# Create and switch to new branch
git checkout -b feature/new-feature

# List branches
git branch -a

# Delete merged branch
git branch -d feature/completed-feature
```

### History & Information
```bash
# View commit history
git log --oneline -10

# View file changes
git diff filename

# View specific commit
git show commit-hash
```