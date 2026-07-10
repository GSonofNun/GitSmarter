# GitHub API Reference for GitSmarter

This document outlines GitHub API features available to enhance GitSmarter as a Git/GitHub client.

## API Options

GitHub offers two APIs:
- **REST API (v3)**: Traditional REST endpoints, well-documented, familiar patterns
- **GraphQL API (v4)**: More flexible queries, fetch exactly what you need in one request

For a native C++ client, REST API is easier to integrate with existing HTTP code in `network.cpp`.

---

## High-Value Features

### 1. Pull Requests (High Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /repos/{owner}/{repo}/pulls` | List PRs with filters (state, head, base) |
| `POST /repos/{owner}/{repo}/pulls` | Create PR |
| `PUT /repos/{owner}/{repo}/pulls/{n}/merge` | Merge PR (merge, squash, rebase) |
| `GET /repos/{owner}/{repo}/pulls/{n}/files` | List changed files |
| `GET /repos/{owner}/{repo}/pulls/{n}/commits` | List commits in PR |
| `GET /repos/{owner}/{repo}/pulls/{n}/reviews` | Get code reviews |
| `POST /repos/{owner}/{repo}/pulls/{n}/reviews` | Submit review (approve, request changes, comment) |
| `GET /repos/{owner}/{repo}/pulls/{n}/comments` | Get review comments on specific lines |
| `POST /repos/{owner}/{repo}/pulls/{n}/comments` | Add line-level comment |

**GitSmarter opportunity**: Full PR workflow - create, review, merge without leaving the app.

### 2. Issues (High Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /repos/{owner}/{repo}/issues` | List issues (filter by state, labels, assignee) |
| `POST /repos/{owner}/{repo}/issues` | Create issue |
| `PATCH /repos/{owner}/{repo}/issues/{n}` | Update (close, assign, label) |
| `GET /issues` | Get authenticated user's issues across all repos |
| `POST /repos/{owner}/{repo}/issues/{n}/comments` | Add comment |
| `GET /repos/{owner}/{repo}/issues/{n}/comments` | List comments |
| `PUT /repos/{owner}/{repo}/issues/{n}/lock` | Lock conversation |

**Note**: Pull requests are issues, but not all issues are pull requests. Shared actions (assignees, labels, milestones) use Issues endpoints.

### 3. GitHub Actions / CI Status (High Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /repos/{owner}/{repo}/actions/runs` | List workflow runs |
| `GET /repos/{owner}/{repo}/actions/runs/{id}` | Get run status (success/fail/pending) |
| `GET /repos/{owner}/{repo}/actions/runs/{id}/jobs` | Get job details |
| `GET /repos/{owner}/{repo}/actions/jobs/{id}/logs` | Download job logs |
| `POST /repos/{owner}/{repo}/actions/runs/{id}/rerun` | Re-run workflow |
| `POST /repos/{owner}/{repo}/actions/runs/{id}/cancel` | Cancel workflow |
| `GET /repos/{owner}/{repo}/commits/{sha}/check-runs` | Check status for a commit |
| `GET /repos/{owner}/{repo}/commits/{sha}/status` | Combined commit status |
| `GET /repos/{owner}/{repo}/actions/runs/{id}/artifacts` | List workflow artifacts |

**GitSmarter opportunity**: Show CI status badges on commits/branches, display workflow run status in commit list.

### 4. Notifications (Medium Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /notifications` | List all notifications |
| `PUT /notifications` | Mark all as read |
| `GET /notifications/threads/{id}` | Get specific notification |
| `PATCH /notifications/threads/{id}` | Mark thread as read/done |
| `PUT /notifications/threads/{id}/subscription` | Subscribe/unsubscribe from thread |
| `GET /repos/{owner}/{repo}/subscription` | Get repo subscription status |
| `PUT /repos/{owner}/{repo}/subscription` | Watch/unwatch repository |

**GitSmarter opportunity**: Built-in notification inbox - see @mentions, PR reviews, issue updates.

### 5. Repository Statistics (Medium Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /repos/{owner}/{repo}/stats/contributors` | Contributor commit activity |
| `GET /repos/{owner}/{repo}/stats/commit_activity` | Weekly commit counts |
| `GET /repos/{owner}/{repo}/stats/participation` | Owner vs others participation |
| `GET /repos/{owner}/{repo}/stats/code_frequency` | Additions/deletions per week |
| `GET /repos/{owner}/{repo}/stats/punch_card` | Hourly commit counts by day |

**Note**: Statistics are cached server-side. First request may return HTTP 202 while GitHub computes them - retry after a short delay.

### 6. Search (Medium Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /search/code?q=...` | Search code in repos |
| `GET /search/commits?q=...` | Search commits |
| `GET /search/issues?q=...` | Search issues/PRs |
| `GET /search/repositories?q=...` | Search repositories |
| `GET /search/users?q=...` | Search users |

**Query qualifiers**: `repo:`, `user:`, `org:`, `language:`, `in:file`, `in:path`, `state:open/closed`

**GitSmarter opportunity**: Powerful search across GitHub from within the app.

### 7. Releases & Tags (Lower Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /repos/{owner}/{repo}/releases` | List releases |
| `GET /repos/{owner}/{repo}/releases/latest` | Get latest release |
| `POST /repos/{owner}/{repo}/releases` | Create release |
| `PATCH /repos/{owner}/{repo}/releases/{id}` | Update release |
| `DELETE /repos/{owner}/{repo}/releases/{id}` | Delete release |
| `POST /repos/{owner}/{repo}/releases/{id}/assets` | Upload release asset |
| `GET /repos/{owner}/{repo}/tags` | List tags |

### 8. User & Activity (Lower Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /user` | Authenticated user profile |
| `GET /users/{username}` | Public user profile |
| `GET /user/starred` | User's starred repos |
| `PUT /user/starred/{owner}/{repo}` | Star a repo |
| `DELETE /user/starred/{owner}/{repo}` | Unstar a repo |
| `GET /users/{username}/repos` | User's repositories |
| `GET /user/orgs` | User's organizations |
| `GET /users/{username}/events` | User's public activity |

### 9. Repository Operations (Lower Priority)

| Endpoint | Capability |
|----------|------------|
| `GET /repos/{owner}/{repo}` | Get repository info |
| `GET /repos/{owner}/{repo}/collaborators` | List collaborators |
| `GET /repos/{owner}/{repo}/branches` | List branches |
| `GET /repos/{owner}/{repo}/branches/{branch}/protection` | Get branch protection rules |
| `POST /repos/{owner}/{repo}/forks` | Fork a repository |
| `GET /repos/{owner}/{repo}/contributors` | List contributors |

---

## Rate Limits

| Auth Type | Rate Limit |
|-----------|------------|
| Unauthenticated | 60/hour (per IP) |
| Personal Access Token | 5,000/hour |
| OAuth App | 5,000/hour |
| GitHub App (installation) | Scales with repos/users |
| GitHub Enterprise Cloud | 15,000/hour |
| GITHUB_TOKEN (Actions) | 1,000/hour per repo |

GitSmarter uses OAuth device flow - 5,000 requests/hour should be sufficient for normal usage.

### Secondary Rate Limits
- Max 100 concurrent requests
- Excessive requests to single endpoint triggers throttling
- Content creation rate-limited to prevent spam
- Response codes: 403 or 429 when exceeded

---

## Authentication

GitSmarter already implements OAuth device flow for GitHub authentication. The access token can be used for all REST API calls:

```
Authorization: Bearer <access_token>
```

Or:
```
Authorization: token <access_token>
```

### Required Scopes by Feature

| Feature | Required Scope |
|---------|---------------|
| Public repo read | (none) |
| Private repo read | `repo` |
| Issues | `repo` (private) or `public_repo` |
| Pull Requests | `repo` (private) or `public_repo` |
| Notifications | `notifications` |
| User data | `read:user` |
| Workflow runs | `actions:read` |

---

## Implementation Priority

### Phase 1 - Core GitHub Integration
1. **CI/CD Status** - Show check status on commits (green checkmark, red X, yellow circle)
2. **Pull Requests** - List, view, create, merge PRs
3. **Issues** - View and create issues

### Phase 2 - Enhanced Workflow
4. **Code Reviews** - View/add PR review comments
5. **Notifications** - Inbox for @mentions and updates
6. **Search** - Find code, commits, issues across GitHub

### Phase 3 - Nice-to-Have
7. **Repository Statistics** - Contributor graphs
8. **Releases** - Create and manage releases
9. **Starred Repos** - Quick access to favorites

---

## API Base URL

```
https://api.github.com
```

All endpoints are relative to this base URL.

### Response Format

GitHub API returns JSON by default. Custom media types available:
- `application/vnd.github+json` - Standard JSON
- `application/vnd.github.raw` - Raw file content
- `application/vnd.github.diff` - Diff format
- `application/vnd.github.patch` - Patch format

### Pagination

List endpoints return max 30 items by default (100 max with `per_page`). Use `Link` header for pagination:
- `page` - Page number (1-indexed)
- `per_page` - Items per page (max 100)

---

## References

- [GitHub REST API Documentation](https://docs.github.com/en/rest)
- [Comparing REST and GraphQL APIs](https://docs.github.com/en/rest/about-the-rest-api/comparing-githubs-rest-api-and-graphql-api)
- [Pull Request Reviews API](https://docs.github.com/en/rest/pulls/reviews)
- [Issues API](https://docs.github.com/en/rest/issues/issues)
- [Workflow Runs API](https://docs.github.com/en/rest/actions/workflow-runs)
- [Notifications API](https://docs.github.com/en/rest/activity/notifications)
- [Repository Statistics API](https://docs.github.com/en/rest/metrics/statistics)
- [Rate Limits](https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api)
- [Search API](https://docs.github.com/en/rest/search)
- [Actions Artifacts API](https://docs.github.com/en/rest/actions/artifacts)
